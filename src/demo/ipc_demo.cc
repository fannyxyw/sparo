#include "command_line.h"
#include "ipc_switches.h"
#include "shared_buffer.cc"
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
constexpr size_t kMaxSendmsgHandles = 128;
static constexpr size_t kChannelBufferSize = 4 * 1024;
static constexpr int kSendmsgFlags = MSG_NOSIGNAL;
static int g_epfd = 0;

#if defined(NDEBUG)

#define HANDLE_EINTR(x)                                                        \
  ({                                                                           \
    decltype(x) eintr_wrapper_result;                                          \
    do {                                                                       \
      eintr_wrapper_result = (x);                                              \
    } while (eintr_wrapper_result == -1 && errno == EINTR);                    \
    eintr_wrapper_result;                                                      \
  })

#else

#define HANDLE_EINTR(x)                                                        \
  ({                                                                           \
    int eintr_wrapper_counter = 0;                                             \
    decltype(x) eintr_wrapper_result;                                          \
    do {                                                                       \
      eintr_wrapper_result = (x);                                              \
    } while (eintr_wrapper_result == -1 && errno == EINTR &&                   \
             eintr_wrapper_counter++ < 100);                                   \
    eintr_wrapper_result;                                                      \
  })

#endif // NDEBUG

#define IGNORE_EINTR(x)                                                        \
  ({                                                                           \
    decltype(x) eintr_wrapper_result;                                          \
    do {                                                                       \
      eintr_wrapper_result = (x);                                              \
      if (eintr_wrapper_result == -1 && errno == EINTR) {                      \
        eintr_wrapper_result = 0;                                              \
      }                                                                        \
    } while (0);                                                               \
    eintr_wrapper_result;                                                      \
  })

} // namespace

ssize_t SendmsgWithHandles(int socket, struct iovec *iov, size_t num_iov,
                           const std::vector<int> &descriptors) {
  char cmsg_buf[CMSG_SPACE(kMaxSendmsgHandles * sizeof(int))];
  struct msghdr msg = {};
  msg.msg_iov = iov;
  msg.msg_iovlen = num_iov;

  msg.msg_control = cmsg_buf;
  msg.msg_controllen = CMSG_LEN(descriptors.size() * sizeof(int));
  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(descriptors.size() * sizeof(int));
  for (size_t i = 0; i < descriptors.size(); ++i) {
    // // DCHECK_GE(descriptors[i].get(), 0);
    reinterpret_cast<int *>(CMSG_DATA(cmsg))[i] = descriptors[i];
  }
  return HANDLE_EINTR(sendmsg(socket, &msg, kSendmsgFlags));
}

ssize_t SocketRecvmsg(int socket, void *buf, size_t num_bytes,
                      std::vector<int> *descriptors, bool block) {
  struct iovec iov = {buf, num_bytes};
  char cmsg_buf[CMSG_SPACE(kMaxSendmsgHandles * sizeof(int))];
  struct msghdr msg = {};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);
  ssize_t result =
      HANDLE_EINTR(recvmsg(socket, &msg, block ? 0 : MSG_DONTWAIT));
  if (result < 0) {

    printf("sdfasd:read:errcoded:%d\n", errno);
    return result;
  }

  printf("sdfasd:read\n");

  if (msg.msg_controllen == 0) {
    return result;
  }
  descriptors->clear();
  for (cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg;
       cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
      size_t payload_length = cmsg->cmsg_len - CMSG_LEN(0);
      // // DCHECK_EQ(payload_length % sizeof(int), 0u);
      size_t num_fds = payload_length / sizeof(int);
      const int *fds = reinterpret_cast<int *>(CMSG_DATA(cmsg));
      for (size_t i = 0; i < num_fds; ++i) {
        int fd(fds[i]);
        descriptors->emplace_back(std::move(fd));
      }
    }
  }

  return result;
}

int CreateSealedMemFD(size_t size) {
  // CHECK_GT(size, 0u);
  // CHECK_EQ(size % base::GetPageSize(), 0u);
  int fd = (syscall(__NR_memfd_create, "mojo_channel_linux",
                    MFD_CLOEXEC | MFD_ALLOW_SEALING));

  if (ftruncate(fd, size) < 0) {
    // PLOG(ERROR) << "Unable to truncate memfd for shared memory channel";
    return {};
  }
  constexpr int kMemFDSeals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW;
  // We make sure to use F_SEAL_SEAL to prevent any further changes to the
  // seals and F_SEAL_SHRINK guarantees that we won't accidentally decrease
  // the size, and similarly F_SEAL_GROW for increasing size.
  if (fcntl(fd, F_ADD_SEALS, kMemFDSeals) < 0) {
    // PLOG(ERROR) << "Unable to seal memfd for shared memory channel";
    return {};
  }

  return fd;
}

#ifndef EFD_ZERO_ON_WAKE
#define EFD_ZERO_ON_WAKE O_NOFOLLOW
#endif

static int CreateWriteNotifier() {
  static constexpr int kEfdFlags = EFD_CLOEXEC | EFD_NONBLOCK;
  static bool zero_on_wake_supported = []() -> bool {
    int fd = syscall(__NR_eventfd2, 0, kEfdFlags | EFD_ZERO_ON_WAKE);
    return fd > 0;
  }();

  bool use_zero_on_wake = zero_on_wake_supported;
  int extra_flags = use_zero_on_wake ? EFD_ZERO_ON_WAKE : 0;
  int fd = syscall(__NR_eventfd2, 0, kEfdFlags | extra_flags);
  if (fd < 0) {
    return -1;
  }

  return fd;
}

bool Notify(int event_fd) {
  uint64_t value = 1;
  ssize_t res = HANDLE_EINTR(write(event_fd, &value, sizeof(value)));
  return res == sizeof(value);
}

bool Clear(int event_fd) {
  uint64_t value = 0;
  ssize_t res = HANDLE_EINTR(
      read(event_fd, reinterpret_cast<void *>(&value), sizeof(value)));
  return res == sizeof(value);
}

int CloseEPFD() {
  if (g_epfd > 0) {
    close(g_epfd);
    g_epfd = 0;
  }
  return 0;
}

int WaitReadable(int event_fd) {
  if (g_epfd == 0) {
    g_epfd = epoll_create(1);
    if (g_epfd == -1) {
      perror("epoll_create");
      return 7;
    }
  }
  struct epoll_event epevent = {0};
  epevent.events = EPOLLIN;
  epevent.data.ptr = NULL;
  if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, event_fd, &epevent)) {
    CloseEPFD();
    perror("epoll_ctl");
    return 8;
  }

  /*****************处理异步请求事件********/
  int i = 0;
  int event_num = 1;
  while (i < event_num) {
    int64_t finished_aio = 0;
    if (epoll_wait(g_epfd, &epevent, 1, -1) != 1) {
      perror("epoll_wait");
      CloseEPFD();
      return 9;
    }

    Clear(event_fd);
    break;
  }

  if (epoll_ctl(g_epfd, EPOLL_CTL_DEL, event_fd, &epevent)) {
    CloseEPFD();
    perror("epoll_ctl");
    return 8;
  }
  // printf("WaitReadable finished on main process ...\n");
  return 0;
}

namespace {
static constexpr char kChild[] = "child";
}


std::vector<std::string> SplitString(const std::string& string,
                                     char delimiter) {
  std::vector<std::string> result;
  if (string.empty())
    return result;

  size_t start = 0;
  while (start != std::string::npos) {
    size_t end = string.find_first_of(delimiter, start);

    std::string part;
    if (end == std::string::npos) {
      part = string.substr(start);
      start = std::string::npos;
    } else {
      part = string.substr(start, end - start);
      start = end + 1;
    }

    result.push_back(part);
  }
  return result;
}

void LaunchProcess(const std::vector<std::string> &argv) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    exit(1);
  } else if (pid == 0) { // child process
    std::vector<char *> argv_cstr;
    argv_cstr.reserve(argv.size() + 1);
    for (auto &arg : argv)
      argv_cstr.push_back(const_cast<char *>(arg.c_str()));
    argv_cstr.push_back(nullptr);
    int null_fd = (HANDLE_EINTR(open("/dev/null", O_RDONLY)));
    if (null_fd < 0) {
      // RAW_LOG(ERROR, "Failed to open /dev/null");
      _exit(127);
    }

    int new_fd = HANDLE_EINTR(dup2(null_fd, STDIN_FILENO));
    if (new_fd != STDIN_FILENO) {
      // RAW_LOG(ERROR, "Failed to dup /dev/null for stdin");
      _exit(127);
    }

    if (setpgid(0, 0) < 0) {
      // RAW_LOG(ERROR, "setpgid failed");
      _exit(127);
    }

    execvp(argv_cstr[0], argv_cstr.data());
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  CommandLine command_line(argc, argv);
  if (command_line.HasSwitch(switches::kLegacyClientFd)) {
    int client_fd = std::atoi(
        command_line.GetSwitchValue(switches::kLegacyClientFd).data());
    int host_fd = std::atoi(
        command_line.GetSwitchValue(switches::kHostIpczTransportFd).data());
    close(client_fd);
    printf("Child client_fd:%d, process stared: %d, ppid: %d !\n", client_fd, getpid(), getppid());
    char channel_buffer[kChannelBufferSize];

    std::vector<int> memfd(2);

    ssize_t read_result = SocketRecvmsg(host_fd, channel_buffer, sizeof(channel_buffer), &memfd, true);
    printf("Read fd: %d, read_result:%ld\n", memfd[0], read_result);

  int ppid = 0;
    int kk = 0;
  std::unique_ptr<SharedBuffer> buffer =
      SharedBuffer::Create(memfd[0], kChannelBufferSize);
  buffer->Initialize();
  for (;;) {
    ++kk;
    sleep(1);
    if (kk % 2 == 0) {
      ppid = getppid();
      if (ppid == 1) {
        printf("Parent process has exited.\n");
        break;
      }
      auto err_code = buffer->TryWrite(&kk, sizeof(kk));
      if (err_code != SharedBuffer::Error::kSuccess) {
        printf("Write failed, error code: %d, pid: %d\n", err_code, getpid());
      } else {
        Notify(memfd[1]);
      }
    }
  }

    return 0;
  }

  int socket_pair[2] = {0};
  // signal(SIGINT, h);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair) == -1) {
    printf("Error, socketpair create failed, errno(%d): %s\n", errno,
           strerror(errno));
    return EXIT_FAILURE;
  }

  printf("main:%d\n", socket_pair[0]);
  command_line.AppendSwitch(switches::kLegacyClientFd,
                            std::to_string(socket_pair[0]));
  command_line.AppendSwitch(switches::kHostIpczTransportFd,
                            std::to_string(socket_pair[1]));
  LaunchProcess(command_line.GetArgs());

  std::vector<int> memfd(2);
  int index = 0;
  memfd[index++] = CreateSealedMemFD(128);
  memfd[index++] = CreateWriteNotifier();
  int client_fd = socket_pair[0];
  char channel_buffer[kChannelBufferSize] = {0};
  iovec iov = {channel_buffer, sizeof(channel_buffer)};
  SendmsgWithHandles(client_fd, &iov, 1, memfd);
  printf("Write fd: %d\n", memfd[0]);

  int kk = 0;
  std::unique_ptr<SharedBuffer> buffer =
      SharedBuffer::Create(memfd[0], kChannelBufferSize);
  buffer->Initialize();
  for (;;) {
    ++kk;
    sleep(1);
    if (1) {
      WaitReadable(memfd[1]);
      if (buffer->TryLockForReading()) {
        std::vector<uint8_t> read_buf;
        size_t len = buffer->usable_len();
        read_buf.resize(len);
        uint32_t bytes_read = 0;
        do {
          auto read_res = buffer->TryReadLocked(read_buf.data(),
                                                read_buf.size(), &bytes_read);
          if (read_res == SharedBuffer::Error::kControlCorruption) {
            // TODO: OnError
            printf("Parent read value: %d, size: %d\n", read_buf[0],
                   bytes_read);
            break;
          }
        } while (0);

        buffer->UnlockForReading();
        if (bytes_read > 0) {
          printf("Parent read value: %d, size: %d\n", read_buf[0], bytes_read);
        }
      }
    }
  }

  CloseEPFD();
  return EXIT_SUCCESS;
}
