#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rtc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gal Anonim");
MODULE_DESCRIPTION("A simple Hello world LKM!");
MODULE_VERSION("0.1");

static int bufferSize;

static int currentReadIndex = 0;
static int currentWriteIndex = 0;

struct mmap_info {
  int *data;
};

struct mmap_info *info;

/* Declarations for command line arguements to be passed to module
	The module_param() macro takes 3 arguments: the name of the variable,
 	its type
	and permissions for the corresponding file in sysfs.
*/
module_param(bufferSize, int, 0000);

static int my_open(struct inode* inode, struct file* filp)
{
	pr_info("my_open\n");
	
  filp->private_data = info;
	return 0;
}

static int my_close(struct inode* inodep, struct file* filp)
{
  pr_info("my_close\n");
  
  return 0;
}

static ssize_t my_read(struct file* filp, char* buffer, size_t count, loff_t* position)
{
  pr_info("my_read in kernel space\n");
	
  if(currentReadIndex > bufferSize)
  {
		pr_warn("Current index to read exceeds the size/bounds of the buffer EXITING\n");
		return ENOMEM; /*Not enough memory in buffer */
	}

  currentReadIndex %= bufferSize;

  if(copy_to_user(buffer, info->data + currentReadIndex, sizeof(int)) != 0)
  {
		pr_warn("ERROR: copy_to_user did not write all bytes to user buffer\n");
		return ENOMEM;
	}
  else
  {
		pr_info("SUCCESS: copy_to_user wrote all bytes to user buffer\n");
	}

	++currentReadIndex;

  return count;
}

static ssize_t my_write(struct file* file, const char __user* buffer, size_t count, loff_t* ppos)
{
  pr_info("my_write in kernel space\n");

	if(currentWriteIndex > bufferSize)
  {
		pr_warn("Current index to write to exceeds bounds/size of buffer.\n");
		return ENOMEM;
	}

  currentWriteIndex %= bufferSize;
	/*Write to the currentWriteIndex - th row, one char in a column at a time */
	if( copy_from_user(info->data + currentWriteIndex, buffer, sizeof(int)) != 0)
  {
			/* Error did not write all bytes*/
		pr_warn("ERROR: copy_from_user did NOT read all bytes from user buffer\n");
		return ENOMEM; 
	}
  else
  {
		pr_info("SUCCESS: copy_from_user read all bytes from user buffer \n");
	}

  ++currentWriteIndex;

  return count;
}

void my_vma_open(struct vm_area_struct *vma)
{
  pr_info("My VMA open, virt %lx, phys %lx\n",
    vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void my_vma_close(struct vm_area_struct *vma)
{
  pr_info("My VMA close.\n");
}

/* First page access. */
static vm_fault_t my_vm_fault(struct vm_fault *vmf)
{
	pr_info("my_vm_fault\n");
	
  info = (struct mmap_info *)vmf->vma->vm_private_data;
	if (info->data)
  {
		struct page *page = virt_to_page(info->data);
		get_page(page);
		vmf->page = page;
	}
	return 0;
}

static struct vm_operations_struct my_vm_ops =
{
    .open = my_vma_open,
    .close = my_vma_close,
    .fault = my_vm_fault,
};

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	pr_info("my_mmap\n");
	vma->vm_ops = &my_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = filp->private_data;
	my_vma_open(vma);
	return 0;
}

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .read    = my_read,
    .write   = my_write,
    .release = my_close,
    .mmap = my_mmap,
};

struct miscdevice pipe_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "aaa_kernel",
    .fops  = &my_fops,
};

static int __init hello_start(void)
{
  int error;

  pr_info("Loading hello module...\n");
  pr_info("Hello world\n");

  error = misc_register(&pipe_device);
  if (error)
  {
    pr_err("Failed to register pipe device\n");
    return error;
  }

  pr_info("Allocating buffer with size %d\n",bufferSize);

  info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	info->data = (int *)get_zeroed_page(GFP_KERNEL);

  pr_info("Successfully created buffer\n");

  return 0;
}

static void __exit hello_end(void)
{
  pr_info("Goodbye Mr.\n");

  free_page((unsigned long)info->data);
  kfree(info);
	pr_info("SUCCESSFULLY FREED NUMPIPE buffer\n");
  
  misc_deregister(&pipe_device);
	pr_info("SUCCESSFULLY DEREGISTERED NUMPIPE  misc device\n");
}

module_init(hello_start);
module_exit(hello_end);
