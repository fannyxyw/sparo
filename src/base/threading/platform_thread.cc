#include "base/threading/platform_thread.h"

#include "base/dcheck_is_on.h"

#include <atomic>
#include <bits/stdint-intn.h>
#include <chrono>
#include <ratio>
#include <string>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <memory>

namespace base {
void InitThreading();
void TerminateOnThread();
size_t GetDefaultThreadStackSize(const pthread_attr_t& attributes);
}  // namespace base

namespace internal {

void InvalidateTidCache();
}  // namespace internal

namespace {

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

// Store the thread ids in local storage since calling the SWI can be
// expensive and PlatformThread::CurrentId is used liberally.
thread_local pid_t g_thread_id = -1;

// A boolean value that indicates that the value stored in |g_thread_id| on the
// main thread is invalid, because it hasn't been updated since the process
// forked.
//
// This used to work by setting |g_thread_id| to -1 in a pthread_atfork handler.
// However, when a multithreaded process forks, it is only allowed to call
// async-signal-safe functions until it calls an exec() syscall. However,
// accessing TLS may allocate (see crbug.com/1275748), which is not
// async-signal-safe and therefore causes deadlocks, corruption, and crashes.
//
// It's Atomic to placate TSAN.
std::atomic<bool> g_main_thread_tid_cache_valid = false;

// Tracks whether the current thread is the main thread, and therefore whether
// |g_main_thread_tid_cache_valid| is relevant for the current thread. This is
// also updated by PlatformThread::CurrentId().
thread_local bool g_is_main_thread = true;

class InitAtFork {
 public:
  InitAtFork() {
    pthread_atfork(nullptr, nullptr, internal::InvalidateTidCache);
  }
};

#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)


struct ThreadParams {
  ThreadParams() = default;

  PlatformThread::Delegate* delegate = nullptr;
  bool joinable = false;
  ThreadType thread_type = ThreadType::kDefault;
  // MessagePumpType message_pump_type = MessagePumpType::DEFAULT;
};


void* ThreadFunc(void* params) {
  PlatformThread::Delegate* delegate = nullptr;

  {
    std::unique_ptr<ThreadParams> thread_params(
        static_cast<ThreadParams*>(params));

    delegate = thread_params->delegate;
  }

  base::InitThreading();

  delegate->ThreadMain();

  base::TerminateOnThread();
  return nullptr;
}

bool CreateThread(size_t stack_size,
                  bool joinable,
                  PlatformThread::Delegate* delegate,
                  PlatformThreadHandle* thread_handle,
                  ThreadType thread_type) {

  pthread_attr_t attributes;
  pthread_attr_init(&attributes);

  // Pthreads are joinable by default, so only specify the detached
  // attribute if the thread should be non-joinable.
  if (!joinable)
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

  // Get a better default if available.
  if (stack_size == 0)
    stack_size = base::GetDefaultThreadStackSize(attributes);

  if (stack_size > 0)
    pthread_attr_setstacksize(&attributes, stack_size);

  std::unique_ptr<ThreadParams> params(new ThreadParams);
  params->delegate = delegate;
  params->joinable = joinable;
  params->thread_type = thread_type;

  pthread_t handle;
  int err = pthread_create(&handle, &attributes, ThreadFunc, params.get());
  bool success = !err;
  if (success) {
    // ThreadParams should be deleted on the created thread after used.
    std::ignore = params.release();
  } else {
    // Value of |handle| is undefined if pthread_create fails.
    handle = 0;
    errno = err;
  }
  *thread_handle = PlatformThreadHandle(handle);

  pthread_attr_destroy(&attributes);

  return success;
}


} // namespace


namespace internal {

void InvalidateTidCache() {
  g_main_thread_tid_cache_valid.store(false, std::memory_order_relaxed);
}

}  // namespace internal

// static
PlatformThreadId PlatformThreadBase::CurrentId() {
  // Pthreads doesn't have the concept of a thread ID, so we have to reach down
  // into the kernel.
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
  // Workaround false-positive MSAN use-of-uninitialized-value on
  // thread_local storage for loaded libraries:
  // https://github.com/google/sanitizers/issues/1265
  // MSAN_UNPOISON(&g_thread_id, sizeof(pid_t));
  // MSAN_UNPOISON(&g_is_main_thread, sizeof(bool));
  static InitAtFork init_at_fork;
  if (g_thread_id == -1 ||
      (g_is_main_thread &&
       !g_main_thread_tid_cache_valid.load(std::memory_order_relaxed))) {
    // Update the cached tid.
    g_thread_id = static_cast<pid_t>(syscall(__NR_gettid));
    // If this is the main thread, we can mark the tid_cache as valid.
    // Otherwise, stop the current thread from always entering this slow path.
    if (g_thread_id == getpid()) {
      g_main_thread_tid_cache_valid.store(true, std::memory_order_relaxed);
    } else {
      g_is_main_thread = false;
    }
  } else {
#if DCHECK_IS_ON()
    if (g_thread_id != syscall(__NR_gettid)) {
      // RAW_LOG(
      //     FATAL,
      //     "Thread id stored in TLS is different from thread id returned by "
      //     "the system. It is likely that the process was forked without going "
      //     "through fork().");
    }
#endif
  }
  return g_thread_id;
#elif BUILDFLAG(IS_ANDROID)
  // Note: do not cache the return value inside a thread_local variable on
  // Android (as above). The reasons are:
  // - thread_local is slow on Android (goes through emutls)
  // - gettid() is fast, since its return value is cached in pthread (in the
  //   thread control block of pthread). See gettid.c in bionic.
  return gettid();
#elif BUILDFLAG(IS_FUCHSIA)
  thread_local static zx_koid_t id =
      GetKoid(*zx::thread::self()).value_or(ZX_KOID_INVALID);
  return id;
#elif BUILDFLAG(IS_SOLARIS) || BUILDFLAG(IS_QNX)
  return pthread_self();
#elif BUILDFLAG(IS_NACL) && defined(__GLIBC__)
  return pthread_self();
#elif BUILDFLAG(IS_NACL) && !defined(__GLIBC__)
  // Pointers are 32-bits in NaCl.
  return reinterpret_cast<int32_t>(pthread_self());
#elif BUILDFLAG(IS_POSIX) && BUILDFLAG(IS_AIX)
  return pthread_self();
#elif BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_AIX)
  return reinterpret_cast<int64_t>(pthread_self());
#endif
}

// static
PlatformThreadHandle PlatformThreadBase::CurrentHandle() {
  return PlatformThreadHandle(pthread_self());
}

#if !BUILDFLAG(IS_APPLE)
// static
void PlatformThreadBase::YieldCurrentThread() {
  sched_yield();
}
#endif  // !BUILDFLAG(IS_APPLE)


void PlatformThreadBase::SetName(const std::string& name) {
  // SetNameCommon(name);

  // On linux we can get the thread names to show up in the debugger by setting
  // the process name for the LWP.  We don't want to do this for the main
  // thread because that would rename the process, causing tools like killall
  // to stop working.
  if (PlatformThread::CurrentId() == getpid())
    return;

  // http://0pointer.de/blog/projects/name-your-threads.html
  // Set the name for the LWP (which gets truncated to 15 characters).
  // Note that glibc also has a 'pthread_setname_np' api, but it may not be
  // available everywhere and it's only benefit over using prctl directly is
  // that it can set the name of threads other than the current thread.
  int err = prctl(PR_SET_NAME, name.c_str());
  // We expect EPERM failures in sandboxed processes, just ignore those.
  if (err < 0 && errno != EPERM) {
    // DPLOG(ERROR) << "prctl(PR_SET_NAME)";
  }
    }

  // Gets the thread name, if previously set by SetName.
const char* PlatformThreadBase::GetName() {
  return "";
}

// static
void PlatformThreadBase::Sleep(uint32_t duration) {
  struct timespec sleep_time, remaining;

  // Break the duration into seconds and nanoseconds.
  // NOTE: TimeDelta's microseconds are int64s while timespec's
  // nanoseconds are longs, so this unpacking must prevent overflow.
  sleep_time.tv_sec = static_cast<time_t>(std::chrono::seconds(
      duration / std::chrono::milliseconds::period::den).count());
  duration -= std::chrono::seconds(sleep_time.tv_sec).count();
  sleep_time.tv_nsec = static_cast<long>(duration * 1000 * 1000);

  while (nanosleep(&sleep_time, &remaining) == -1 && errno == EINTR)
    sleep_time = remaining;
}


bool PlatformThreadBase::CreateWithType(
      size_t stack_size,
      Delegate* delegate,
      PlatformThreadHandle* thread_handle,
      ThreadType thread_type) {
  return CreateThread(stack_size, true /* joinable thread */, delegate,
                      thread_handle, thread_type);
}

void PlatformThreadBase::Join(PlatformThreadHandle thread_handle) {
  // Joining another thread may block the current thread for a long time, since
  // the thread referred to by |thread_handle| may still be running long-lived /
  // blocking tasks.
  // base::internal::ScopedBlockingCallWithBaseSyncPrimitives scoped_blocking_call(
  //     FROM_HERE, base::BlockingType::MAY_BLOCK);
  pthread_join(thread_handle.platform_handle(), nullptr);
}

// static
void PlatformThreadBase::Detach(PlatformThreadHandle thread_handle) {
  pthread_detach(thread_handle.platform_handle());
}


namespace base {
  // for linux
size_t GetDefaultThreadStackSize(const pthread_attr_t& attributes) {
#if !defined(THREAD_SANITIZER) && defined(__GLIBC__)
  // Generally glibc sets ample default stack sizes, so use the default there.
  return 0;
#elif !defined(THREAD_SANITIZER)
  // Other libcs (uclibc, musl, etc) tend to use smaller stacks, often too small
  // for chromium. Make sure we have enough space to work with here. Note that
  // for comparison glibc stacks are generally around 8MB.
  return 2 * (1 << 20);
#else
  // ThreadSanitizer bloats the stack heavily. Evidence has been that the
  // default stack size isn't enough for some browser tests.
  return 2 * (1 << 23);  // 2 times 8192K (the default stack size on Linux).
#endif
}

void InitThreading() {}

void TerminateOnThread() {}
}  // namespace base
