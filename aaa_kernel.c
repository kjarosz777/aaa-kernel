#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rtc.h>
#include <linux/sched.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gal Anonim");
MODULE_DESCRIPTION("A simple Hello world LKM!");
MODULE_VERSION("0.1");

static int my_open(struct inode* inode, struct file* file)
{
  printk(KERN_INFO "Opened aaa_kernel misc char device\n");
  return 0;
}

static int my_close(struct inode* inodep, struct file* filp)
{
  printk(KERN_INFO "Closed aaa_kernel misc char device\n");
  return 0;
}

static ssize_t my_read(struct file* filp, char* buffer, size_t count, loff_t* position)
{
  printk(KERN_INFO "Using read() function of aaa_kernel in kernel space\n");
  return 0;
}

static ssize_t my_write(struct file* file, const char __user* buffer, size_t count, loff_t* ppos)
{
  printk(KERN_INFO "Using write() function of aaa_kernel module\n");
  return count;
}

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .read    = my_read,
    .write   = my_write,
    .release = my_close,
};

struct miscdevice pipe_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "aaa_kernel",
    .fops  = &my_fops,
};

static int __init hello_start(void)
{
  int error;

  printk(KERN_INFO "Loading hello module...\n");
  printk(KERN_INFO "Hello world\n");

  error = misc_register(&pipe_device);
  if (error)
  {
    printk(KERN_ERR "Failed to register pipe device\n");
    return error;
  }

  return 0;
}

static void __exit hello_end(void)
{
  printk(KERN_INFO "Goodbye Mr.\n");

  misc_deregister(&pipe_device);
}

module_init(hello_start);
module_exit(hello_end);
