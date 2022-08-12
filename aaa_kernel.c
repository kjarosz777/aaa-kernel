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

int * pipeBuffer;
static int bufferSize;

static int currentReadIndex = 0;
static int currentWriteIndex = 0;

enum { BUFFER_SIZE = 4 };

struct mmap_info {
	char *data;
};


/* First page access. */
static vm_fault_t vm_fault(struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;

	pr_info("vm_fault\n");
	info = (struct mmap_info *)vmf->vma->vm_private_data;
	if (info->data) {
		page = virt_to_page(info->data);
		get_page(page);
		vmf->page = page;
	}
	return 0;
}

/* Declarations for command line arguements to be passed to module
	The module_param() macro takes 3 arguments: the name of the variable,
 	its type
	and permissions for the corresponding file in sysfs.
*/
module_param(bufferSize, int, 0000);

static int my_open(struct inode* inode, struct file* filp)
{
  // printk(KERN_INFO "Opened aaa_kernel misc char device\n");
  // return 0;
  struct mmap_info *info;

	pr_info("open\n");
	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	// pr_info("virt_to_phys = 0x%llx\n", (unsigned long long)virt_to_phys((void *)info));
	info->data = (char *)get_zeroed_page(GFP_KERNEL);
	memcpy(info->data, "asdf", BUFFER_SIZE);
	filp->private_data = info;
	return 0;
}

static int release(struct inode *inode, struct file *filp)
{
	struct mmap_info *info;

	pr_info("release\n");
	info = filp->private_data;
	free_page((unsigned long)info->data);
	kfree(info);
	filp->private_data = NULL;
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
	
  if(currentReadIndex > bufferSize){
		printk(KERN_WARNING "Current index to read exceeds the size/bounds of the buffer EXITING\n");
		return ENOMEM; /*Not enough memory in buffer */
	}

  currentReadIndex %= bufferSize;

  if( copy_to_user(buffer, pipeBuffer + currentReadIndex, sizeof(int)) !=0 ){
		printk(KERN_WARNING "ERROR: copy_to_user did not write all bytes to user buffer\n");
		return ENOMEM;
	}else{
		printk(KERN_WARNING "SUCCESS: copy_to_user wrote all bytes to user buffer\n");
	}
	++currentReadIndex;

  return count;
}

static ssize_t my_write(struct file* file, const char __user* buffer, size_t count, loff_t* ppos)
{
  printk(KERN_INFO "Using write() function of aaa_kernel module\n");

	if(currentWriteIndex > bufferSize){
		printk(KERN_WARNING "Current index to write to exceeds bounds/size of buffer.\n");
		return ENOMEM;
	}

  currentWriteIndex %= bufferSize;
	/*Write to the currentWriteIndex - th row, one char in a column at a time */
	if( copy_from_user(pipeBuffer + currentWriteIndex, buffer, sizeof(int)) != 0){
			/* Error did not write all bytes*/
		printk(KERN_WARNING "ERROR: copy_from_user did NOT read all bytes from user buffer\n");
		return ENOMEM; 
	}else{
		printk(KERN_NOTICE "SUCCESS: copy_from_user read all bytes from user buffer \n");
	}

  	++currentWriteIndex;

  return count;
}

void my_vma_open(struct vm_area_struct *vma)
{
  printk(KERN_NOTICE "My VMA open, virt %lx, phys %lx\n",
    vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void my_vma_close(struct vm_area_struct *vma)
{
  printk(KERN_NOTICE "My VMA close.\n");
}

static struct vm_operations_struct my_vm_ops =
{
    .open = my_vma_open,
    .close = my_vma_close,
    .fault = vm_fault,
};

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	pr_info("mmap\n");
	vma->vm_ops = &my_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = filp->private_data;
	my_vma_open(vma);
	return 0;
}

// int my_mmap (struct file* file, struct vm_area_struct * vma)
// {
//   // vma->vm_ops = &my_vm_ops;
//   // my_vma_open(vma);

//     int ret = 0;
//     struct page *page = NULL;
//     unsigned long size = (vma->vm_end - vma->vm_start);

//     if (size <= 0) {
//       return -EINVAL; 
//     } 

//     printk(KERN_NOTICE "size: %lu\n", size);

//     uintptr_t pipeBuffer2 = (uintptr_t)pipeBuffer;

//     printk(KERN_NOTICE "pipeBuffer: %p, pipeBuffer2: %lx, (*int)pipeBuffer2: %p, *int)pipeBuffer3: %lx, \n",
//       pipeBuffer, pipeBuffer2, (int*)(pipeBuffer2), (int*)(pipeBuffer2));

//     page = virt_to_page((unsigned long)pipeBuffer + (vma->vm_pgoff << PAGE_SHIFT));
//     // page = virt_to_page((pipeBuffer2 << PAGE_SHIFT));
    
//     printk(KERN_NOTICE "vm_start %lx, vm_end %lx, vm_pgoff: %lx, phys %lx\n",
//       vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_pgoff << PAGE_SHIFT);
//     printk(KERN_NOTICE "pipeBuffer: %p, ULpipeBuffer: %lx, uintptr_t_pipeBuffer: %lx, page: %p\n",
//       pipeBuffer, (unsigned long)pipeBuffer, (uintptr_t)pipeBuffer, page);

//     ret = remap_pfn_range(vma, vma->vm_start, page_to_pfn(page), size, vma->vm_page_prot);
//     if (ret < 0)
//     {
//       printk(KERN_ERR "remap_pfn_range failed, ret: %d\n", ret);
//     }
//     else
//     {
//       printk(KERN_NOTICE "remap_pfn_range success, ret: %d\n", ret);
//     }

//   return ret;
// }

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = release,
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

  printk(KERN_INFO "Loading hello module...\n");
  printk(KERN_INFO "Hello world\n");

  error = misc_register(&pipe_device);
  if (error)
  {
    printk(KERN_ERR "Failed to register pipe device\n");
    return error;
  }

  printk(KERN_NOTICE "Allocating buffer with size %d\n",bufferSize);

  pipeBuffer = kmalloc(bufferSize * sizeof(int),GFP_KERNEL);
  printk(KERN_NOTICE "Successfully created buffer\n");

  return 0;
}

static void __exit hello_end(void)
{
  printk(KERN_INFO "Goodbye Mr.\n");

  kfree(pipeBuffer);
	printk(KERN_NOTICE "SUCCESSFULLY FREED NUMPIPE buffer\n");
  misc_deregister(&pipe_device);
	printk(KERN_NOTICE "SUCCESSFULLY DEREGISTERED NUMPIPE  misc device\n");
}

module_init(hello_start);
module_exit(hello_end);
