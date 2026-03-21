#ifndef BASE_THREADING_PLATFORM_THREAD_H_
#define BASE_THREADING_PLATFORM_THREAD_H_

#include <bits/stdint-uintn.h>
#include <chrono>
#include <ratio>
#include <stddef.h>

#include <iosfwd>
#include <type_traits>

#include <pthread.h>
#include "build/build_config.h"
#include "build/buildflag.h"
#include "base/base_export.h"

// Used for logging. Always an integer value.
#if BUILDFLAG(IS_WIN)
typedef DWORD PlatformThreadId;
#elif BUILDFLAG(IS_FUCHSIA)
typedef zx_koid_t PlatformThreadId;
#elif BUILDFLAG(IS_APPLE)
typedef mach_port_t PlatformThreadId;
#elif BUILDFLAG(IS_POSIX)
typedef pid_t PlatformThreadId;
#endif
static_assert(std::is_integral_v<PlatformThreadId>, "Always an integer value.");

// Used to operate on threads.
class PlatformThreadHandle {
 public:
#if BUILDFLAG(IS_WIN)
  typedef void* Handle;
#elif BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
  typedef pthread_t Handle;
#endif

  constexpr PlatformThreadHandle() : handle_(0) {}

  explicit constexpr PlatformThreadHandle(Handle handle) : handle_(handle) {}

  bool is_equal(const PlatformThreadHandle& other) const {
    return handle_ == other.handle_;
  }

  bool is_null() const {
    return !handle_;
  }

  Handle platform_handle() const {
    return handle_;
  }

 private:
  Handle handle_;
};

const PlatformThreadId kInvalidThreadId(0);

// Valid values for `thread_type` of Thread::Options, SimpleThread::Options,
// and SetCurrentThreadType(), listed in increasing order of importance.
//
// It is up to each platform-specific implementation what these translate to.
// Callers should avoid setting different ThreadTypes on different platforms
// (ifdefs) at all cost, instead the platform differences should be encoded in
// the platform-specific implementations. Some implementations may treat
// adjacent ThreadTypes in this enum as equivalent.
//
// Reach out to //base/task/OWNERS (scheduler-dev@chromium.org) before changing
// thread type assignments in your component, as such decisions affect the whole
// of Chrome.
//
// Refer to PlatformThreadTest.SetCurrentThreadTypeTest in
// platform_thread_unittest.cc for the most up-to-date state of each platform's
// handling of ThreadType.
enum class ThreadType : int {
  // Suitable for threads that have the least urgency and lowest priority, and
  // can be interrupted or delayed by other types.
  kBackground,
  // Suitable for threads that are less important than normal type, and can be
  // interrupted or delayed by threads with kDefault type.
  kUtility,
  // Suitable for threads that produce user-visible artifacts but aren't
  // latency sensitive. The underlying platform will try to be economic
  // in its usage of resources for this thread, if possible.
  kResourceEfficient,
  // Default type. The thread priority or quality of service will be set to
  // platform default. In Chrome, this is suitable for handling user
  // interactions (input), only display and audio can get a higher priority.
  kDefault,
  // Suitable for threads which are critical to compositing the foreground
  // content.
  kCompositing,
  // Suitable for display critical threads.
  kDisplayCritical,
  // Suitable for low-latency, glitch-resistant audio.
  kRealtimeAudio,
  kMaxValue = kRealtimeAudio,
};


// A namespace for low-level thread functions.
class BASE_EXPORT PlatformThreadBase {
 public:
  // Implement this interface to run code on a background thread.  Your
  // ThreadMain method will be called on the newly created thread.
  class BASE_EXPORT Delegate {
   public:
    virtual void ThreadMain() = 0;

   protected:
    virtual ~Delegate() = default;
  };

  PlatformThreadBase() = delete;
  PlatformThreadBase(const PlatformThreadBase&) = delete;
  PlatformThreadBase& operator=(const PlatformThreadBase&) = delete;


  // Gets the current thread id, which may be useful for logging purposes.
  static PlatformThreadId CurrentId();

  // Get the handle representing the current thread. On Windows, this is a
  // pseudo handle constant which will always represent the thread using it and
  // hence should not be shared with other threads nor be used to differentiate
  // the current thread from another.
  static PlatformThreadHandle CurrentHandle();

  // Yield the current thread so another thread can be scheduled.
  //
  // Note: this is likely not the right call to make in most situations. If this
  // is part of a spin loop, consider base::Lock, which likely has better tail
  // latency. Yielding the thread has different effects depending on the
  // platform, system load, etc., and can result in yielding the CPU for less
  // than 1us, or many tens of ms.
  static void YieldCurrentThread();

  // Sleeps for the specified duration (real-time; ignores time overrides).
  // Note: The sleep duration may be in base::Time or base::TimeTicks, depending
  // on platform. If you're looking to use this in unit tests testing delayed
  // tasks, this will be unreliable - instead, use
  // base::test::TaskEnvironment with MOCK_TIME mode.
  static void Sleep(uint32_t duration);

  // Sets the thread name visible to debuggers/tools. This will try to
  // initialize the context for current thread unless it's a WorkerThread.
  static void SetName(const std::string& name);

  // Gets the thread name, if previously set by SetName.
  static const char* GetName();

  // Creates a new thread.  The `stack_size` parameter can be 0 to indicate
  // that the default stack size should be used.  Upon success,
  // `*thread_handle` will be assigned a handle to the newly created thread,
  // and `delegate`'s ThreadMain method will be executed on the newly created
  // thread.
  // NOTE: When you are done with the thread handle, you must call Join to
  // release system resources associated with the thread.  You must ensure that
  // the Delegate object outlives the thread.
  static bool Create(size_t stack_size,
                     Delegate* delegate,
                     PlatformThreadHandle* thread_handle) {
    return CreateWithType(stack_size, delegate, thread_handle,
                          ThreadType::kDefault);
  }

  // CreateWithType() does the same thing as Create() except the priority and
  // possibly the QoS of the thread is set based on `thread_type`.
  // `pump_type_hint` must be provided if the thread will be using a
  // MessagePumpForUI or MessagePumpForIO as this affects the application of
  // `thread_type`.
  static bool CreateWithType(
      size_t stack_size,
      Delegate* delegate,
      PlatformThreadHandle* thread_handle,
      ThreadType thread_type);


  // Joins with a thread created via the Create function.  This function blocks
  // the caller until the designated thread exits.  This will invalidate
  // `thread_handle`.
  static void Join(PlatformThreadHandle thread_handle);

  // Detaches and releases the thread handle. The thread is no longer joinable
  // and `thread_handle` is invalidated after this call.
  static void Detach(PlatformThreadHandle thread_handle);
};

// Alias to the correct platform-specific class based on preprocessor directives
#if BUILDFLAG(IS_APPLE)
using PlatformThread = PlatformThreadApple;
#elif BUILDFLAG(IS_CHROMEOS)
using PlatformThread = PlatformThreadChromeOS;
#elif BUILDFLAG(IS_LINUX)
using PlatformThread = PlatformThreadBase;
#else
using PlatformThread = PlatformThreadBase;
#endif

#endif // BASE_THREADING_PLATFORM_THREAD_H_
