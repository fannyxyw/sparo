// 客户端		client
#include "command_line.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <thread>
#include <unistd.h>

/**
 * 设置 file describe 为非阻塞模式
 * @param fd 文件描述
 * @return 返回0成功, 返回-1失败
 */
static int make_socket_non_blocking(int fd) {
  int flags, s;
  // 获取当前flag
  flags = fcntl(fd, F_GETFL, 0);
  if (-1 == flags) {
    perror("Get fd status");
    return -1;
  }

  flags |= O_NONBLOCK;

  // 设置flag
  s = fcntl(fd, F_SETFL, flags);
  if (-1 == s) {
    perror("Set fd status");
    return -1;
  }
  return 0;
}

int SetTCPNoDelay(int fd, int no_delay) {
  int on = no_delay ? 1 : 0;
  int rv =
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
  return rv;
}

int run_client(int16_t port) {
  // 创建socket
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in seraddr;
  inet_pton(AF_INET, "127.0.0.1", &seraddr.sin_addr.s_addr);
  seraddr.sin_family = AF_INET;
  seraddr.sin_port = htons(port);

  // 连接服务器
  int ret = connect(fd, (struct sockaddr *)&seraddr, sizeof(seraddr));
  if (ret == -1) {
    perror("connect");
    return -1;
  }
  int num = 0;
  while (1) {
    char sendBuf[1024] = {0};
    // fgets(sendBuf, sizeof(sendBuf), stdin);
    // sprintf(sendBuf, "send data %d", num++);
    fgets(sendBuf, sizeof(sendBuf), stdin);
    write(fd, sendBuf, strlen(sendBuf) + 1);
    bzero(sendBuf, sizeof(sendBuf));
    // 接收
    int len = read(fd, sendBuf, sizeof(sendBuf));
    if (len == -1) {
      perror("read");
      return -1;
    } else if (len > 0) {
      printf("read data: %s\n", sendBuf);
    } else {
      printf("服务器已经断开连接...\n");
      break;
    }
    // sleep(1);
  }
  close(fd);
  exit(0);
}

#include "eintr_wrapper.h"

int WaitSocketForRead(int fd, int16_t timeout) {
  fd_set read_fds;
  struct timeval tv {
    .tv_sec = 0, .tv_usec = timeout * 1000
  };

  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  return HANDLE_EINTR(select(fd + 1, &read_fds, nullptr, nullptr, &tv));
}


int32_t ReadWithoutBlocking(int fd, char* buf, int16_t size) {
    size_t bytes_read_ = 0;
  while (bytes_read_ < sizeof(buf)) {
    ssize_t rv =
        HANDLE_EINTR(read(fd, buf + bytes_read_, sizeof(buf) - bytes_read_));
    if (rv < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        // PLOG(ERROR) << "read() failed";
        // CloseSocket(fd);
        return rv;
      } else {
        // It would block, so we just return and continue to watch for the next
        // opportunity to read.
        return 0;
      }
    } else if (!rv) {
      // No more data to read.  It's time to process the message.
      return 0;
    } else {
      bytes_read_ += rv;
    }
  }

  return bytes_read_;
}

int SetupSocketOnly() {
  int sock = socket(PF_UNIX, SOCK_STREAM, 0);
//   PCHECK(sock >= 0) << "socket() failed";

  //DCHECK(base::SetNonBlocking(sock)) << "Failed to make non-blocking socket.";
//   int rv = SetCloseOnExec(sock);
//   DCHECK_EQ(0, rv) << "Failed to set CLOEXEC on socket.";

  return sock;
}


// Set up a sockaddr appropriate for messaging.
// bool SetupSockAddr(const std::string& path,
//                    SockaddrUn* addr,
//                    socklen_t* socklen) {
//   addr->sun_family = AF_UNIX;
// #if BUILDFLAG(IS_MAC)
//   // Allow the use of the entire length of sun_path, without reservation for a
//   // NUL terminator. The socklen parameter to bind and connect encodes the
//   // length of the sockaddr structure, and xnu does not require sun_path to be
//   // NUL-terminated. This is not portable, but it’s OK on macOS, and allows
//   // maximally-sized paths on a platform where the singleton socket path is
//   // already long. 11.5 xnu-7195.141.2/bsd/kern/uipc_usrreq.c unp_bind,
//   // unp_connect.
//   if (path.length() > std::size(addr->sun_path))
//     return false;

//   // On input to the kernel, sun_len is ignored and overwritten by the value of
//   // the passed-in socklen parameter. 11.5
//   // xnu-7195.141.2/bsd/kern/uipc_syscalls.c getsockaddr[_s]; note that the
//   // field is sa_len and not sun_len there because it occurs in generic code
//   // referring to sockaddr before being specialized into sockaddr_un or any
//   // other address family's sockaddr structure.
//   //
//   // Since the length needs to be computed for socklen anyway, just populate
//   // sun_len correctly.
//   addr->sun_len =
//       offsetof(std::remove_pointer_t<decltype(addr)>, sun_path) + path.length();

//   *socklen = addr->sun_len;
//   memcpy(addr->sun_path, path.c_str(), path.length());
// #else
//   // The portable version: NUL-terminate sun_path and don’t touch sun_len (which
//   // may not even exist).
//   if (path.length() >= std::size(addr->sun_path))
//     return false;
//   *socklen = sizeof(*addr);
//   base::strlcpy(addr->sun_path, path.c_str(), std::size(addr->sun_path));
// #endif
//   return true;
// }


bool WriteToSocket(int fd, const char *message, size_t length) {
//   DCHECK(message);
//   DCHECK(length);
  size_t bytes_written = 0;
  do {
    ssize_t rv = HANDLE_EINTR(
        write(fd, message + bytes_written, length - bytes_written));
    if (rv < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // The socket shouldn't block, we're sending so little data.  Just give
        // up here, since NotifyOtherProcess() doesn't have an asynchronous api.
        // LOG(ERROR) << "ProcessSingleton would block on write(), so it gave up.";
        return false;
      }
    //   PLOG(ERROR) << "write() failed";
      return false;
    }
    bytes_written += rv;
  } while (bytes_written < length);

  return true;
}


const int kServerPort = 8888;

int run_server(void) {
  const char *const local_addr = "127.0.0.1";
  struct sockaddr_in server_addr = {0};
  int listen_fd = 0;
  int result = 0;
  listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (-1 == listen_fd) {
    perror("Open listen socket");
    return -1;
  }
  /* Enable address reuse */
  int on = 1;
  result = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (-1 == result) {
    perror("Set socket");
    return 0;
  }
  server_addr.sin_family = AF_INET;
  inet_aton(local_addr, &(server_addr.sin_addr));
  server_addr.sin_port = htons(kServerPort);
  result = bind(listen_fd, (const struct sockaddr *)&server_addr,
                sizeof(server_addr));
  if (-1 == result) {
    perror("Bind port");
    return 0;
  }

  result = listen(listen_fd, 16);
  if (-1 == result) {
    perror("Start listen");
    return 0;
  }

  while (true) {
    if (WaitSocketForRead(listen_fd, 500) > 0) {
      struct sockaddr_in client_addr;
      socklen_t client_addr_len = sizeof(client_addr);
      int client_fd =
          accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
      if (client_fd == -1) {
        perror("accept4 失败");
        close(listen_fd);
        exit(EXIT_FAILURE);
      } else {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("新客户端连接：IP = %s, 端口 = %d, 客户端 fd = %d\n", client_ip,
               ntohs(client_addr.sin_port), client_fd);
      }
    }
  }
  return 0;
}
namespace switches {
constexpr char kServerPort[] = "port";
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


void PrintToStderr(const char* output) {
  // NOTE: This code MUST be async-signal safe (it's used by in-process
  // stack dumping signal handler). NO malloc or stdio is allowed here.
    write(STDERR_FILENO, output, strlen(output));
//   USE(return_val);
}

// NOTE: code from sandbox/linux/seccomp-bpf/demo.cc.
char* itoa_r(intptr_t i, char* buf, size_t sz, int base, size_t padding) {
  // Make sure we can write at least one NUL byte.
  size_t n = 1;
  if (n > sz) return nullptr;

  if (base < 2 || base > 16) {
    buf[0] = '\0';
    return nullptr;
  }

  char* start = buf;

  uintptr_t j = i;

  // Handle negative numbers (only for base 10).
  if (i < 0 && base == 10) {
    // This does "j = -i" while avoiding integer overflow.
    j = static_cast<uintptr_t>(-(i + 1)) + 1;

    // Make sure we can write the '-' character.
    if (++n > sz) {
      buf[0] = '\0';
      return nullptr;
    }
    *start++ = '-';
  }

  // Loop until we have converted the entire number. Output at least one
  // character (i.e. '0').
  char* ptr = start;
  do {
    // Make sure there is still enough space left in our output buffer.
    if (++n > sz) {
      buf[0] = '\0';
      return nullptr;
    }

    // Output the next digit.
    *ptr++ = "0123456789abcdef"[j % base];
    j /= base;

    if (padding > 0) padding--;
  } while (j > 0 || padding > 0);

  // Terminate the output with a NUL character.
  *ptr = '\0';

  // Conversion to ASCII actually resulted in the digits being in reverse
  // order. We can't easily generate them in forward order, as we can't tell
  // the number of characters needed until we are done converting.
  // So, now, we reverse the string (except for the possible "-" sign).
  while (--ptr > start) {
    char ch = *ptr;
    *ptr = *start;
    *start++ = ch;
  }
  return buf;
}

void StackDumpSignalHandler(int signal, siginfo_t* info, void* void_context) {
  // NOTE: This code MUST be async-signal safe.
  // NO malloc or stdio is allowed here.

  // Record the fact that we are in the signal handler now, so that the rest
  // of StackTrace can behave in an async-signal-safe manner.
//   int in_signal_handler = 1;

  PrintToStderr("Received signal ");
  char buf[1024] = {0};
  itoa_r(signal, buf, sizeof(buf), 10, 0);
  PrintToStderr(buf);
  if (signal == SIGBUS) {
    if (info->si_code == BUS_ADRALN)
      PrintToStderr(" BUS_ADRALN ");
    else if (info->si_code == BUS_ADRERR)
      PrintToStderr(" BUS_ADRERR ");
    else if (info->si_code == BUS_OBJERR)
      PrintToStderr(" BUS_OBJERR ");
    else
      PrintToStderr(" <unknown> ");
  } else if (signal == SIGFPE) {
    if (info->si_code == FPE_FLTDIV)
      PrintToStderr(" FPE_FLTDIV ");
    else if (info->si_code == FPE_FLTINV)
      PrintToStderr(" FPE_FLTINV ");
    else if (info->si_code == FPE_FLTOVF)
      PrintToStderr(" FPE_FLTOVF ");
    else if (info->si_code == FPE_FLTRES)
      PrintToStderr(" FPE_FLTRES ");
    else if (info->si_code == FPE_FLTSUB)
      PrintToStderr(" FPE_FLTSUB ");
    else if (info->si_code == FPE_FLTUND)
      PrintToStderr(" FPE_FLTUND ");
    else if (info->si_code == FPE_INTDIV)
      PrintToStderr(" FPE_INTDIV ");
    else if (info->si_code == FPE_INTOVF)
      PrintToStderr(" FPE_INTOVF ");
    else
      PrintToStderr(" <unknown> ");
  } else if (signal == SIGILL) {
    if (info->si_code == ILL_BADSTK)
      PrintToStderr(" ILL_BADSTK ");
    else if (info->si_code == ILL_COPROC)
      PrintToStderr(" ILL_COPROC ");
    else if (info->si_code == ILL_ILLOPN)
      PrintToStderr(" ILL_ILLOPN ");
    else if (info->si_code == ILL_ILLADR)
      PrintToStderr(" ILL_ILLADR ");
    else if (info->si_code == ILL_ILLTRP)
      PrintToStderr(" ILL_ILLTRP ");
    else if (info->si_code == ILL_PRVOPC)
      PrintToStderr(" ILL_PRVOPC ");
    else if (info->si_code == ILL_PRVREG)
      PrintToStderr(" ILL_PRVREG ");
    else
      PrintToStderr(" <unknown> ");
  } else if (signal == SIGSEGV) {
    if (info->si_code == SEGV_MAPERR)
      PrintToStderr(" SEGV_MAPERR ");
    else if (info->si_code == SEGV_ACCERR)
      PrintToStderr(" SEGV_ACCERR ");
    else
      PrintToStderr(" <unknown> ");
  }
  if (signal == SIGBUS || signal == SIGFPE || signal == SIGILL ||
      signal == SIGSEGV) {
    itoa_r(reinterpret_cast<intptr_t>(info->si_addr), buf,
                     sizeof(buf), 16, 12);
    PrintToStderr(buf);
  }
  PrintToStderr("\n");
//   if (dump_stack_in_signal_handler) {
//     debug::StackTrace().Print();
//     PrintToStderr("[end of stack trace]\n");
//   }

  if (::signal(signal, SIG_DFL) == SIG_ERR) _exit(1);
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

int main(int argc, char *argv[]) {
  auto cmd = SplitString(argv[1], ' ');
  LaunchProcess(cmd);
  return 0;

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_flags = static_cast<int>(SA_RESETHAND | SA_SIGINFO);
  action.sa_sigaction = &StackDumpSignalHandler;
  sigemptyset(&action.sa_mask);

  CommandLine command_line(argc, argv);
  if (command_line.HasSwitch(switches::kServerPort)) {
    run_client(atoi(command_line.GetSwitchValue(switches::kServerPort).data()));
  } else {
    std::thread server(run_server);
    command_line.AppendSwitch(switches::kServerPort,
                              std::to_string(kServerPort));
    LaunchProcess(command_line.GetArgs());
    for (;;) {
      sleep(1);
    }
  }

  return 0;
}
