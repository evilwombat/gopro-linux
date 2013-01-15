
/*
 * amba_audiomem.c
 *
 * History:
 *	2011/9/13 - [Eric Lee] created file
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

#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/aipc/aipc.h>

extern int ipc_iaudio_get_audio_buf_info(unsigned int *base_addr, unsigned int *size);
extern int i_audio2_api_init(void);
extern void i_audio2_api_cleanup(void);


static unsigned int aucbuf_baseaddr = 0;
static unsigned int aucbuf_size = 0;

struct ambaaudio_dev {
	struct miscdevice *misc_dev;
};

static struct ambaaudio_dev amba_audio_dev = {NULL};


static long audiomem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
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

static int audiomem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rval;
	unsigned long size;

	if (ipc_iaudio_get_audio_buf_info(&aucbuf_baseaddr, &aucbuf_size) < 0) {
		printk("ipc_iaudio_get_audio_buf_info() fail\n");
		rval = -EINVAL;
		goto Done;
	}
	printk("%s: audcbuf_baseaddr=0x%x, audcbuf_size=0x%x\n",__func__, aucbuf_baseaddr, aucbuf_size);
	
	size = vma->vm_end - vma->vm_start;
	if (size != aucbuf_size) {
		rval = -EINVAL;
		goto Done;
	}

	vma->vm_page_prot = phys_mem_access_prot(filp, vma->vm_pgoff,
						 size, vma->vm_page_prot);

	vma->vm_pgoff = ((int)aucbuf_baseaddr) >> PAGE_SHIFT;
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

static int audiomem_open(struct inode *inode, struct file *filp)
{
printk("%s\n",__func__);
	return 0;
}

static int audiomem_release(struct inode *inode, struct file *filp)
{
printk("%s\n",__func__);
	return 0;
}

static struct file_operations audiomem_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = audiomem_ioctl,
	.mmap = audiomem_mmap,
	.open = audiomem_open,
	.release = audiomem_release,
};

static struct miscdevice amba_audio_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "amba_audiomem",
	.fops = &audiomem_fops,
};

struct platform_device amba_audiomem = {
	.name			= "amba_audiomem",
	.id			= -1,
	.dev			= {
    .platform_data		= NULL,
	}
};
EXPORT_SYMBOL(amba_audiomem);

static int __devinit ambaaudio_probe(struct platform_device *pdev)
{
	int err = 0;

	if (amba_audio_dev.misc_dev != NULL){
		dev_err(&pdev->dev, "Amba_Audio already exists. Skip operation!\n");
		return 0;
	}

	platform_set_drvdata(pdev, &amba_audio_dev);

	amba_audio_dev.misc_dev = &amba_audio_device;
	
	err = misc_register(amba_audio_dev.misc_dev);
	if (err){
		dev_err(&pdev->dev, "failed to misc_register Amba_Audio.\n");
		goto err_fail;
	}

	printk("Probe %s successfully\n",amba_audio_dev.misc_dev->name);
	i_audio2_api_init();
	
	return 0;

err_fail:
	misc_deregister(amba_audio_dev.misc_dev);
	amba_audio_dev.misc_dev=NULL;
	return err;
}


static int __devexit ambaaudio_remove(struct platform_device *pdev)
{
	i_audio2_api_cleanup();
	misc_deregister(amba_audio_dev.misc_dev);
	amba_audio_dev.misc_dev=NULL;
	
	return 0;
}

static struct platform_driver ambaaudio_driver = {
	.probe = ambaaudio_probe,
	.remove = ambaaudio_remove,
	.driver = { .name = "amba_audiomem" }
};

static int __init __ambaaudiomem_init(void)
{
	return platform_driver_register(&ambaaudio_driver);
}

static void __exit __ambaaudiomem_exit(void)
{
	platform_driver_unregister(&ambaaudio_driver);
}

module_init(__ambaaudiomem_init);
module_exit(__ambaaudiomem_exit);

MODULE_AUTHOR("Eric Lee <cylee@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Audio driver");
MODULE_LICENSE("GPL");
