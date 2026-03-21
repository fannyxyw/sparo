#include "command_line.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
namespace {
  static constexpr char kChild[] = "child";
}

void LaunchProcess(int argc, char* argv[]) {
    pid_t pid = fork();  // 创建子进程
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {  // 子进程
      if (argc > 1) {
        exit(0);
      }
        execvp(argv[0], argv);  // 子进程执行命令
        exit(1);
    } else {  // 父进程
        printf("Command executed, parent resumes.\n");
    }
}

int main(int argc, char* argv[]) {
    CommandLine command_line(argc, argv);
    if (command_line.HasSwitch(kChild)) {
      sleep(1);
      printf("child\n");
      return 0;
    }

    printf("main\n");
    command_line.AppendSwitch(kChild);
    const CommandLine::StringVector& argp = command_line.GetArgs();

    std::vector<char*> argv_cstr;
    argv_cstr.reserve(argp.size() + 1);
    for (auto& arg : argp)
      argv_cstr.push_back(const_cast<char*>(arg.c_str()));
    argv_cstr.push_back(nullptr);
    LaunchProcess(argc, argv_cstr.data());
    return 0;
}