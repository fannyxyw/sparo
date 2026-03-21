#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
// #include <format>
#include <iomanip>
#include <sstream>

#include <algorithm>

#include <spdlog/spdlog.h>

#pragma pack(1)
struct MyStruct {
  int32_t len;
  uint8_t type;
  uint8_t c;
  uint16_t d;
  uint16_t e;
};
#pragma pack()

void log_info(const std::string& message) {
  spdlog::info(message);
}

void log_error(const std::string& message) {
  spdlog::error(message);
}
template <typename T>
std::string to_hex_string(T value) {
  return std::format("{:x}", value);
}

bool receive_struct(int sock, MyStruct& my_struct) {
  ssize_t bytes_received =
      recv(sock, &my_struct, sizeof(MyStruct), MSG_WAITALL);
  if (bytes_received < sizeof(MyStruct)) {
    log_info("Received less than expected: " + std::to_string(bytes_received));
    return false;
  }
  return true;
}

// Set socket to non-blocking mode
void set_nonblocking(int sockfd) {
  int flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

// Add this function to handle client data
bool handle_client_data(int client_fd) {
  MyStruct recv_struct{};

  while (true) {
    // Try to receive data
    ssize_t bytes_received = recv(client_fd, &recv_struct, sizeof(MyStruct), 0);

    if (bytes_received == 0) {
      // Client closed connection
      log_info("Client disconnected");
      return false;
    }

    if (bytes_received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more data available right now
        return true;
      }
      // Error occurred
      log_error("Receive error: " + std::string(strerror(errno)));
      return false;
    }

    if (bytes_received == sizeof(MyStruct)) {
      log_info("Received: len=" + std::to_string(recv_struct.len) +
               " type=" + to_hex_string(recv_struct.type) + " c=0x" +
               to_hex_string(recv_struct.c) + " d=0x" +
               to_hex_string(recv_struct.d) + " e=0x" +
               to_hex_string(recv_struct.e));

      // Prepare and send response
      MyStruct send_struct{};
      send_struct.len = sizeof(MyStruct);
      send_struct.type = 2;
      send_struct.c = 8;
      send_struct.d = 9;
      send_struct.e = 13;

      uint8_t additional_data[] = {0x01, 0x02, 0x0F, 0x04};

      // Send response (with error handling)
      ssize_t sent = send(client_fd, &send_struct, sizeof(MyStruct), 0);
      if (sent < 0) {
        log_error("Send failed: " + std::string(strerror(errno)));
        return false;
      }

      sent = send(client_fd, additional_data, sizeof(additional_data), 0);
      if (sent < 0) {
        log_error("Send additional data failed: " +
                  std::string(strerror(errno)));
        return false;
      }

      log_info("Sent data: " +
               std::to_string(sizeof(MyStruct) + sizeof(additional_data)));
    }
  }
}

void start_server(const char* host = "0.0.0.0", int port = 10086) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    log_error("Socket creation failed");
    exit(1);
  }

  // Set server socket to non-blocking
  set_nonblocking(server_fd);

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    log_error("Setsockopt failed");
    exit(1);
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    log_error("Bind failed");
    exit(1);
  }

  if (listen(server_fd, 3) < 0) {
    log_error("Listen failed");
    exit(1);
  }

  // Create epoll instance
  int epollfd = epoll_create1(0);
  if (epollfd == -1) {
    log_error("Epoll creation failed");
    exit(1);
  }

  struct epoll_event ev, events[10];
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
    log_error("Failed to add server socket to epoll");
    exit(1);
  }

  log_info("Server listening on " + std::string(host) + ":" +
           std::to_string(port));

  while (true) {
    int nfds = epoll_wait(epollfd, events, 10, -1);
    if (nfds == -1) {
      log_error("Epoll wait failed");
      break;
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == server_fd) {
        // Handle new connection
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        int client_fd =
            accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);

        if (client_fd == -1) {
          log_error("Accept failed: " + std::string(strerror(errno)));
          continue;
        }

        set_nonblocking(client_fd);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        log_info("Connected by " + std::string(client_ip));

        ev.events = EPOLLIN | EPOLLET;  // Edge triggered
        ev.data.fd = client_fd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
          log_error("Failed to add client to epoll");
          close(client_fd);
        }
      } else {
        // Handle client data
        int client_fd = events[n].data.fd;

        if (!handle_client_data(client_fd)) {
          epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, nullptr);
          close(client_fd);
          log_info("Connection closed");
        }
      }
    }
  }

  close(epollfd);
  close(server_fd);
  log_info("Server closed");
}

void InitSpdglog() {
  // Customize msg format for all loggers
  spdlog::set_pattern("[%Y%m%d %H:%M:%S.%e] [%^%L%$] [thread %t] %v");
  spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR,
               SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

  spdlog::warn("Easy padding in numbers like {:08d}", 12);
  spdlog::critical(
      "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
  spdlog::info("Support for floats {:03.2f}", 1.23456);
  spdlog::info("Positional args are {1} {0}..", "too", "supported");
  spdlog::info("{:>8} aligned, {:<8} aligned", "right", "left");

  // Runtime log levels
  spdlog::set_level(spdlog::level::debug);
}

int main() {
    std::hash<std::string> hash_fn;
    std::string str = "Test";
    std::size_t str_hash = hash_fn(str);
    std::cout << "Hash of " << str << " is " << str_hash << std::endl;
    InitSpdglog();
  try {
    start_server();
  } catch (const std::exception& e) {
    log_error("Server terminated: " + std::string(e.what()));
  }
  return 0;
}