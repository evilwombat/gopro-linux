/*
 * amba_heap_mem.c.c
 *
 * History:
 *	2012/04/07 - [Keny Huang] created file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/mm.h>

extern int ipc_i_heapmem_get_info(unsigned char **base_addr, unsigned int *size);
extern int ipc_i_heapmem_init(void);
extern void ipc_i_heapmem_cleanup(void);

static unsigned char *heap_baseaddr = NULL;
static unsigned int heap_size = 0;
struct amba_heap_mem_dev {
	struct miscdevice *misc_dev;
};

static struct amba_heap_mem_dev amba_heap_dev = {NULL};

static long amba_heap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
/*	int rval;
//	mutex_lock(&iav_mutex);

	switch (cmd) {
	case IAV_IOC_GET_UCODE_INFO:
		rval = copy_to_user((ucode_load_info_t __user*)arg,
			dsp_get_ucode_info(), sizeof(ucode_load_info_t)) ? -EFAULT : 0;
		break;

	case IAV_IOC_GET_UCODE_VERSION:
		rval = copy_to_user((ucode_version_t __user*)arg,
			dsp_get_ucode_version(), sizeof(ucode_version_t)) ? -EFAULT : 0;
		break;

	case IAV_IOC_UPDATE_UCODE:
		if (ucode_user_start == 0)
			rval = -EPERM;
		else {
			clean_d_cache((void*)ucode_user_start, dsp_get_ucode_size());
			rval = 0;
		}
		break;

	default:
		rval = -ENOIOCTLCMD;
		break;
	}

//	mutex_unlock(&iav_mutex);
	return rval;
	*/
	printk("%s\n",__func__);
	return 0;
}

#define pgprot_noncached(prot) \
       __pgprot_modify(prot, L_PTE_MT_MASK, L_PTE_MT_UNCACHED)

static pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot)
{
	if (file->f_flags & O_DSYNC){
		printk("phys_mem_access_prot: set as noncached\n");
		return pgprot_noncached(vma_prot);
	}

	return vma_prot;
}

static int amba_heap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rval;
	unsigned long size;
	int baseaddr;

//printk("%s\n",__func__);
	if(ipc_i_heapmem_get_info(&heap_baseaddr, &heap_size)<0){
		printk("ipc_i_heapmem_get_info() fail\n");
		rval = -EINVAL;
		goto Done;
	}
	
	size = vma->vm_end - vma->vm_start;
	if(size==heap_size) {
		printk("%s: heap_baseaddr=%p, heap_size=%x\n",__func__, heap_baseaddr, heap_size);
		baseaddr=(int)heap_baseaddr;
	} else {
		rval = -EINVAL;
		goto Done;
	}

	//if(filp->f_flags & O_SYNC){
	//	vma->vm_flags |= VM_IO;
	//}
	//vma->vm_flags |= VM_RESERVED;

	vma->vm_page_prot = phys_mem_access_prot(filp, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	vma->vm_pgoff = (baseaddr) >> PAGE_SHIFT;
	if ((rval = remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			size,
			vma->vm_page_prot)) < 0) {
		goto Done;
	}

	rval = 0;

Done:
	return rval;
}

static int amba_heap_open(struct inode *inode, struct file *filp)
{
printk("%s\n",__func__);
	return 0;
}

static int amba_heap_release(struct inode *inode, struct file *filp)
{
printk("%s\n",__func__);
	return 0;
}

static struct file_operations amba_heap_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_heap_ioctl,
	.mmap = amba_heap_mmap,
	.open = amba_heap_open,
	.release = amba_heap_release,
};

static struct miscdevice amba_heap_mem_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "amba_heap_mem",
	.fops = &amba_heap_fops,
};

struct platform_device amba_heap_mem = {
	.name			= "amba_heap_mem",
	.id			= -1,
	.dev			= {
		.platform_data		= NULL,
	}
};
EXPORT_SYMBOL(amba_heap_mem);

static int __devinit amba_heap_probe(struct platform_device *pdev)
{
	int err = 0;

		
	if(amba_heap_dev.misc_dev!=NULL){
		dev_err(&pdev->dev, "Amba_Heap_mem already exists. Skip operation!\n");
		return 0;
	}

	platform_set_drvdata(pdev, &amba_heap_dev);

	amba_heap_dev.misc_dev = &amba_heap_mem_device;
	
	err = misc_register(amba_heap_dev.misc_dev);
	if (err){
		dev_err(&pdev->dev, "failed to misc_register Amba_Heap_mem.\n");
		goto err_fail;
	}

	printk("Probe %s successfully\n",amba_heap_dev.misc_dev->name);
	ipc_i_heapmem_init();
	
	return 0;

err_fail:
	misc_deregister(amba_heap_dev.misc_dev);
	amba_heap_dev.misc_dev=NULL;
	return err;
}


static int __devexit amba_heap_remove(struct platform_device *pdev)
{
	ipc_i_heapmem_cleanup();
	misc_deregister(amba_heap_dev.misc_dev);
	amba_heap_dev.misc_dev=NULL;
	
	return 0;
}

static struct platform_driver amba_heap_mem_driver = {
	.probe = amba_heap_probe,
	.remove = amba_heap_remove,
	.driver = { .name = "amba_heap_mem" }
};

static int __init __amba_heap_init(void)
{
	return platform_driver_register(&amba_heap_mem_driver);
}

static void __exit __amba_heap_exit(void)
{
	platform_driver_unregister(&amba_heap_mem_driver);
}

module_init(__amba_heap_init);
module_exit(__amba_heap_exit);

MODULE_AUTHOR("Keny Huang <skhuang@ambarella.com>");
MODULE_DESCRIPTION("Ambarella streaming driver");
MODULE_LICENSE("GPL");
