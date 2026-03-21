#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>

#include "./command.h"

#define HELLO_CNT 1

// 主设备号为0，表示动态分配设备号
dev_t dev = 0;
static int major = 0;
static int minor = 0;

static struct cdev* hello_cdev[HELLO_CNT];
static struct class* hello_class = NULL;
static struct device* hello_class_dev[HELLO_CNT];

static int s_val = 0;

int hello_open(struct inode* pnode, struct file* pfile) {
  printk("open file..\n");
  int num = MINOR(pnode->i_rdev);
  if (num >= HELLO_CNT) {
    return -ENODEV;
  }

  pfile->private_data = hello_cdev[num];

  return 0;
}

int hello_release(struct inode* pnode, struct file* pfile) {
  printk("release file ..\n");

  return 0;
}

long hello_ioctl(struct file* pfile, unsigned int cmd, unsigned long val) {
  int ret = 0;

  switch (cmd) {
    case HELLO_RESET: {
      printk("Rev HELLO_RESET cmd\n");
      break;
    }
    case HELLO_READ: {
      printk("Rec HELLO_READ cmd\n");

      ret = copy_to_user(val, &s_val, sizeof(int));
      break;
    }
    case HELLO_WRITE: {
      printk("Rec HELLO_WRITE cmd\n");

      ret = copy_from_user(&s_val, val, sizeof(int));
      break;
    }
    default: {
      printk("unkownd cmd...\n");
      return -EINVAL;
    }
  }

  return ret;
}

// 文件操作结构体
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .release = hello_release,
    //.read = hello_read,
    //.write = hello_write,
    .unlocked_ioctl = hello_ioctl,
};

static void setup_cdev(int index) {
  int err, devno = MKDEV(major, index);

  cdev_init(hello_cdev[index], &fops);
  hello_cdev[index]->owner = THIS_MODULE;
  hello_cdev[index]->ops = &fops;
  err = cdev_add(hello_cdev[index], devno, 1);
  if (err) {
    printk(KERN_NOTICE "Error %d adding hello%d", err, index);
  }
}

static int __init hello_init(void) {
  // 申请设备号，动态or静态
  int ret = 0;
  if (major) {
    // 为字符设备静态申请第一个设备号
    dev = MKDEV(major, minor);
    ret = register_chrdev_region(dev, HELLO_CNT, "hello");
  } else {
    // 为字符设备动态申请一个设备号
    ret = alloc_chrdev_region(&dev, minor, HELLO_CNT, "hello");
    major = MAJOR(dev);
  }

  // 构造cdev设备对象
  int i = 0;
  for (i = 0; i < HELLO_CNT; ++i) {
    hello_cdev[i] = cdev_alloc();
  }

  // 初始化设备对象
  for (minor = 0; minor < HELLO_CNT; ++minor) {
    setup_cdev(minor);
  }

  hello_class = class_create("hello");
  for (minor = 0; minor < HELLO_CNT; ++minor) {
    hello_class_dev[minor] = device_create(
        hello_class, NULL, MKDEV(major, minor), NULL, "hello%d", minor);
  }

  printk(KERN_ALERT "Hello, World, install hello module!\n");

  return 0;
}

static void __exit hello_exit(void) {
  for (minor = 0; minor < HELLO_CNT; ++minor) {
    device_destroy(hello_class, MKDEV(major, minor));
  }
  class_destroy(hello_class);

  // 从内核注销cdev设备对象
  int i = 0;
  for (i = 0; i < HELLO_CNT; ++i) {
    cdev_del(hello_cdev[i]);
  }

  // 回收设备号
  unregister_chrdev_region(dev, HELLO_CNT);

  printk(KERN_ALERT "Goodbye, remove hello module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
