#ifndef INCLUDE_COMMAND_H
#define INCLUDE_COMMAND_H


#include <linux/ioctl.h>


//幻数
#define IOCTL_MAGIC 'x'


//定义命令
#define HELLO_RESET _IO(IOCTL_MAGIC, 1)
#define HELLO_READ _IOR(IOCTL_MAGIC, 2, int)
#define HELLO_WRITE _IOW(IOCTL_MAGIC, 3, int)


#endif
