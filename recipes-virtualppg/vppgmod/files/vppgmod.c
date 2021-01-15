//#define DEBUG

//useful to identify the function that write kernel messages
#ifdef DEBUG
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#else
#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/init.h>

#include "data.h"

#define VIRTUAL_SAMPLE 2048

static dev_t PPG_devt;
static struct class* PPG_class;
struct cdev PPG_cdev_struct;
static int actual_sample = 0;

static ssize_t vppgmod_read(struct file* filp, char __user * buf, size_t count, loff_t* f_pos)
{
  int ret;
  //creating an internal buffer whit the same size of an int
  char kbuf[sizeof(int)];

  //check the buffer dimension
  if (count != sizeof(int))
    return -EINVAL;

  //initializing the buffer at the value to be returned
  *(int*)kbuf = ppg[actual_sample];
  //the buffer will contain the same memory contenent of a value read
  //from the sensor, this method uses 4 bytes on every transaction instead 
  //of 12 bytes for a 32bit integer value converted into string format.

  pr_debug("read value: %u\n", ppg[actual_sample]);

  //update to the next value
  if (actual_sample < VIRTUAL_SAMPLE - 1)
    ++actual_sample;
  else
    actual_sample = 0;

  //copy the buffer to the user space
  ret = copy_to_user(buf, kbuf, sizeof(int));
  if (ret)
    return -EFAULT;

  f_pos += sizeof(int);

  return sizeof(int); //number of bytes read
}

//operations on the module (open, close are NULL by default and not necessary in this case)
static const struct file_operations PPG_fops = {
  .owner = THIS_MODULE,
  .read = vppgmod_read,
};

static int __init vppgmod_init(void)
{
  int to_ret;
  struct device* PPG_device;

  pr_info("Loading vppgmod\n");

  //creating class structure
  PPG_class = class_create(THIS_MODULE, "vppgmod");
  if (IS_ERR(PPG_class))
  {
    pr_err("failed while allocating class\n");
    return PTR_ERR(PPG_class);
  }

  //register the character device
  to_ret = alloc_chrdev_region(&PPG_devt, 0, 1, "vppgmod");
  if (to_ret)
  {
    pr_err("failed while allocating device region\n");
    class_destroy(PPG_class);
    return to_ret;
  }

  pr_info("Got major number: %u\n", MAJOR(PPG_devt));

  //register the device starting from class structure
  PPG_device = device_create(PPG_class, NULL, PPG_devt, NULL, "vppgmod");
  if (IS_ERR(PPG_device))
  {
    pr_err("failed while creating device\n");
    class_destroy(PPG_class);
    unregister_chrdev_region(PPG_devt, 1);
    return PTR_ERR(PPG_device);
  }

  //initialize the cdev structure and add the device to the system
  cdev_init(&PPG_cdev_struct, &PPG_fops);
  to_ret = cdev_add(&PPG_cdev_struct, PPG_devt, 1);
  if (to_ret)
  {
    pr_err("failed while adding device");
    cdev_del(&PPG_cdev_struct);
    device_destroy(PPG_class, PPG_devt);
    class_destroy(PPG_class);
    unregister_chrdev_region(PPG_devt, 1);
    return to_ret;
  }

  pr_debug("vppgmod Loaded!\n");

  return 0;
}

static void __exit vppgmod_exit(void)
{
  pr_info("Cleaning vppgmod\n");

  cdev_del(&PPG_cdev_struct);
  device_destroy(PPG_class, PPG_devt);
  class_destroy(PPG_class);
  unregister_chrdev_region(PPG_devt, 1);
}

module_init(vppgmod_init);
module_exit(vppgmod_exit);

MODULE_AUTHOR("Edward Manca");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual PPG Module");