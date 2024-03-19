#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENT 20
#define READ_BUF_LEN 256

/**
 * 设置 file describe 为非阻塞模式
 * @param fd 文件描述
 * @return 返回0成功，返回-1失败
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
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
  return rv;
}

int BindEvent(struct epoll_event* ev, int epfd, int ev_fd, int op, int flags) {
  ev->data.fd = ev_fd;
  ev->events = flags;
  int result = epoll_ctl(epfd, op, ev_fd, ev);
  if (-1 == result) {
    perror("epoll_ctl");
  }

  return result;
}

int AcceptConnection(int listenfd, int epfd, struct epoll_event* ev) {
  for (;;) {  // 由于采用了边缘触发模式，这里需要使用循环
    struct sockaddr in_addr = {0};
    socklen_t in_addr_len = sizeof(in_addr);
    int accp_fd = accept(listenfd, &in_addr, &in_addr_len);
    if (-1 == accp_fd) {
      break;
    }
    int result = make_socket_non_blocking(accp_fd);
    if (-1 == result) {
      return 0;
    }
    BindEvent(ev, epfd, accp_fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT | EPOLLET);
    SetTCPNoDelay(accp_fd, 1);
  }

  return 0;
}

int ReadData(int client_fd) {
  // 因为采用边缘触发，所以这里需要使用循环。如果不使用循环，程序并不能完全读取到缓存区里面的数据。
  int done = 0;
  for (;;) {
    char buf[READ_BUF_LEN] = {0};
    ssize_t result_len = read(client_fd, buf, sizeof(buf) / sizeof(buf[0]));
    if (-1 == result_len) {
      if (EAGAIN != errno) {
        perror("Read data");
        done = 1;
      }
      break;
    } else if (!result_len) {
      done = 1;
      break;
    }
    write(STDOUT_FILENO, buf, result_len);
  }

  return done;
}

int main() {
  // epoll 实例 file describe
  int epfd = 0;
  int listenfd = 0;
  int result = 0;
  struct epoll_event ev, event[MAX_EVENT];
  // 绑定的地址
  const char* const local_addr = "127.0.0.1";
  struct sockaddr_in server_addr = {0};

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == listenfd) {
    perror("Open listen socket");
    return -1;
  }
  /* Enable address reuse */
  int on = 1;
  // 打开 socket 端口复用, 防止测试的时候出现 Address already in use
  result = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (-1 == result) {
    perror("Set socket");
    return 0;
  }
  int port = 1234;
  server_addr.sin_family = AF_INET;
  inet_aton(local_addr, &(server_addr.sin_addr));
  server_addr.sin_port = htons(port);
  result =
      bind(listenfd, (const struct sockaddr*)&server_addr, sizeof(server_addr));
  if (-1 == result) {
    perror("Bind port");
    return 0;
  }
  result = make_socket_non_blocking(listenfd);
  if (-1 == result) {
    return 0;
  }

  result = listen(listenfd, 200);
  if (-1 == result) {
    perror("Start listen");
    return 0;
  }
  printf("epoll demo server was listen: %d\n", port);

  // 创建epoll实例
  epfd = epoll_create1(0);
  if (1 == epfd) {
    perror("Create epoll instance");
    return 0;
  }
  result = BindEvent(&ev, epfd, listenfd, EPOLL_CTL_ADD, EPOLLIN);
  if (-1 == result) {
    return 0;
  }

  for (;;) {
    int wait_count;
    // 等待事件
    wait_count = epoll_wait(epfd, event, MAX_EVENT, 5);
    if (wait_count == 0) {
      continue;
    }

    for (int i = 0; i < wait_count; i++) {
      uint32_t events = event[i].events;
      int result;
      int event_fd = event[i].data.fd;
      // 判断epoll是否发生错误
      if (events & EPOLLHUP) {
        printf("client has closed:%d\n", event_fd);
        close(event_fd);
        continue;
      } else if (events & EPOLLERR) {
        printf("Epoll has error\n");
        close(event_fd);
        continue;
      } else if (listenfd == event_fd) {
        AcceptConnection(listenfd, epfd, &ev);
        continue;
      } else {
        // 其余事件为 file describe 可以读取
        int done = 0;
        int sockfd = event[i].data.fd;
        if (sockfd < 0) {
          continue;
        }

        if ((events & EPOLLIN) && (ReadData(sockfd))) {
          done = 1;
        }
        if (events & EPOLLOUT) {
          char* buffer = "END666";
          sockfd = event[i].data.fd;
          write(sockfd, buffer, 7);
        }
        if (done) {
          printf("Closed connection\n");
          close(event[i].data.fd);
        }
      }
    }
  }
  close(epfd);
  return 0;
}
