
/*
 * ambastreamdrv.c
 *
 * History:
 *	2011/4/7 - [Keny Huang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
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

extern int ipc_i_streamer_get_iavpool_info(unsigned char **base_addr, unsigned int *size);
extern int ipc_i_streamer_init(void);
extern void ipc_i_streamer_cleanup(void);
extern int ipc_i_preview_get_previewpool_info(unsigned char **base_addr, unsigned int *size);
extern int ipc_i_preview_init(void);
extern void ipc_i_preview_cleanup(void);

static unsigned char *iavpool_baseaddr = NULL;
static unsigned int iavpool_size = 0;
static unsigned char *previewpool_baseaddr = NULL;
static unsigned int previewpool_size = 0;
struct ambastrmem_dev {
	struct miscdevice *misc_dev;
};

static struct ambastrmem_dev amba_strmem_dev = {NULL};

//DEFINE_MUTEX(iav_mutex);

static long ambastrmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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

static int ambastrmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rval;
	unsigned long size;
	int baseaddr;

	//iavpool must map as read-only area!
	//if((vma->vm_flags & VM_WRITE)!=0){
	//	printk("ERR(%s): the memory area must be mapped as READ-ONLY (PROT_READ)!!\n",__func__);
	//	return -EACCES;
	//}
	
//	mutex_lock(&iav_mutex);
//printk("%s\n",__func__);
	if(ipc_i_streamer_get_iavpool_info(&iavpool_baseaddr, &iavpool_size)<0){
		printk("ipc_i_streamer_get_iavpool_info() fail\n");
		rval = -EINVAL;
		goto Done;
	}
	
	if(ipc_i_preview_get_previewpool_info(&previewpool_baseaddr, &previewpool_size)<0){
		printk("ipc_i_preview_get_previewpool_info() fail\n");
		rval = -EINVAL;
		goto Done;
	}
	
	size = vma->vm_end - vma->vm_start;
	if(size==iavpool_size) {
		printk("%s: iavpool_baseaddr=%p, iavpool_size=%x\n",__func__, iavpool_baseaddr, iavpool_size);
		baseaddr=(int)iavpool_baseaddr;
	} else if(size==previewpool_size) {
		printk("%s: previewpool_baseaddr=%p, previewpool_size=%x\n",__func__, previewpool_baseaddr, previewpool_size);
		baseaddr=(int)previewpool_baseaddr;
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

	//ucode_user_start = vma->vm_start;
	rval = 0;

Done:
//	mutex_unlock(&iav_mutex);
	return rval;
}

static int ambastrmem_open(struct inode *inode, struct file *filp)
{
printk("%s\n",__func__);
	return 0;
}

static int ambastrmem_release(struct inode *inode, struct file *filp)
{
printk("%s\n",__func__);
	return 0;
}

static struct file_operations ambastrmem_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ambastrmem_ioctl,
	.mmap = ambastrmem_mmap,
	.open = ambastrmem_open,
	.release = ambastrmem_release,
};

static struct miscdevice amba_strmem_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "amba_streammem",
	.fops = &ambastrmem_fops,
};

struct platform_device amba_streammem = {
	.name			= "amba_streammem",
	.id			= -1,
	.dev			= {
		.platform_data		= NULL,
	}
};
EXPORT_SYMBOL(amba_streammem);

static int __devinit ambastrmem_probe(struct platform_device *pdev)
{
	int err = 0;

		
	if(amba_strmem_dev.misc_dev!=NULL){
		dev_err(&pdev->dev, "Amba_Strmem already exists. Skip operation!\n");
		return 0;
	}

	platform_set_drvdata(pdev, &amba_strmem_dev);

	amba_strmem_dev.misc_dev = &amba_strmem_device;
	
	err = misc_register(amba_strmem_dev.misc_dev);
	if (err){
		dev_err(&pdev->dev, "failed to misc_register Amba_Strmem.\n");
		goto err_fail;
	}

	printk("Probe %s successfully\n",amba_strmem_dev.misc_dev->name);
	ipc_i_streamer_init();
	ipc_i_preview_init();
	
	return 0;

err_fail:
	misc_deregister(amba_strmem_dev.misc_dev);
	amba_strmem_dev.misc_dev=NULL;
	return err;
}


static int __devexit ambastrmem_remove(struct platform_device *pdev)
{
	ipc_i_streamer_cleanup();
	ipc_i_preview_cleanup();
	misc_deregister(amba_strmem_dev.misc_dev);
	amba_strmem_dev.misc_dev=NULL;
	
	return 0;
}

static struct platform_driver ambastrmem_driver = {
	.probe = ambastrmem_probe,
	.remove = ambastrmem_remove,
	.driver = { .name = "amba_streammem" }
};

static int __init __ambastrmem_init(void)
{
	return platform_driver_register(&ambastrmem_driver);
}

static void __exit __ambastrmem_exit(void)
{
	platform_driver_unregister(&ambastrmem_driver);
}

module_init(__ambastrmem_init);
module_exit(__ambastrmem_exit);

MODULE_AUTHOR("Keny Huang <skhuang@ambarella.com>");
MODULE_DESCRIPTION("Ambarella streaming driver");
MODULE_LICENSE("GPL");
