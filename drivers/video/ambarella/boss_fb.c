/*
 * drivers/video/ambarella/boss_fb.c
 *
 * Authors:
 *	Charles Chiou <cchiou@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Copyright (C) 2009-2010, Ambarella Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <asm/sizes.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <plat/fb.h>
#include <linux/aipc/i_util.h>
#include <linux/aipc/lk_util.h>
#include <linux/aipc/i_dsp.h>
#include <plat/ambcache.h>

#if defined(CONFIG_AMBARELLA_IPC)

#define DEBUG_BOSS_FB	0

#if DEBUG_BOSS_FB
#define DEBUG_MSG_FB pr_notice
#else
#define DEBUG_MSG_FB(...)
#endif

#define CCF_NUM			2
#define DEFAULT_FB_NUM		2

extern void cc_rgb565_yuv565(u32 *src, u32 *dst, int pitch,
			     int xres, int yres);

struct boss_video_mem
{
	void *mem;
	unsigned int size;

	void *curmem;
	unsigned int leftover;
	int partitions;
};

/*
 * Extrapolated settings from BOSS (i.e., inherited from remote OS).
 * This frame buffer is going to modify some properties, pre-claim of DSP
 * states are kept here so that it has valid data to operate on and
 * restore to later.
 */
struct boss_fb
{
	struct vdspdrv_osd original;	/* Original setting pre-claim */
	int video_plane;		/* Original video plane on/off */
	void *clut;			/* Original CLUT pointer */

	int width;
	int height;
	int pitch;

	int csc_en;
	unsigned int csc_parms[9];

	void *ccf[CCF_NUM];			/* Color-converted frames */
	int ccfi;			/* Current color-converted frame */
	int nccf;			/* Number of color-converted frame */

	void *smem_start;		/* FB subsystem smem start phys addr */
	void *smem_start_virt;		/* FB subsystem smem start virt addr */
	unsigned int smem_len;		/* FB subsustem smem length */
	int framesize;
	int smem_index;

	unsigned int ccflen;		/* Color-conversion frame length */

	int active;			/* FB is active */

	struct vdspdrv_osd bossed;	/* BOSS setting post-claim */
};

static struct boss_video_mem G_boss_video_mem;
static struct boss_fb G_boss_fb[2];

/*
 * Change color look-up table.
 */
static int ambfb_vdspdrv_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	struct ambarella_platform_fb *pfb =
		(struct ambarella_platform_fb *) info->par;
	struct boss_fb *boss_fb;
	int voutid;

	DEBUG_MSG_FB ("[ipc] ambfb_vdspdrv_setcmap (%d)\n", vdspdrv->active);

	if (vdspdrv->active == 0)
		return -EIO;

	/*
	 * Set new color look-up table.
	 */
	voutid = info->node;
	boss_fb = &G_boss_fb[voutid];
	if (boss_fb->active) {
		vdspdrv_osd_set_clut(voutid, pfb->clut_table);
	}

	return 0;
}

/*
 * Pan display (updated OSD pointer).
 */
static int ambfb_vdspdrv_pan_display(struct fb_var_screeninfo *var,
				     struct fb_info *info)
{
	struct ambarella_platform_fb *pfb =
		(struct ambarella_platform_fb *) info->par;
	struct boss_fb *boss_fb;
	int voutid = info->node;

	boss_fb = &G_boss_fb[voutid];
	boss_fb->smem_index = var->yoffset / boss_fb->height;
		
	if (vdspdrv->active == 0)
		return -EIO;

	pfb->proc_wait_flag++;
	wake_up(&pfb->proc_wait);

	/* Update frame */
	DEBUG_MSG_FB ("[ipc] ambfb_vdspdrv_pan_display (%d)\n", voutid);

	if (boss_fb->active) {
		void *src, *dst;
		int offset;

		offset = info->fix.line_length * var->yoffset;
		src = info->screen_base + offset;
		dst = boss_fb->ccf[boss_fb->ccfi];

		DEBUG_MSG_FB ("[ipc] %dx%d, %d (%08x)\n",
			info->fix.line_length, var->yoffset, offset, offset);

		memcpy (dst, src, boss_fb->framesize);
		ambcache_clean_range (dst, boss_fb->framesize);
		vdspdrv_osd_setbuf(voutid, dst);

		boss_fb->ccfi = (boss_fb->ccfi + 1) % boss_fb->nccf;
	}

	return 0;
}

/*
 * Activate our frame buffer to be displayed by the DSP.
 */
static int ambfb_activate(int voutid)
{
	struct boss_fb *boss_fb = &G_boss_fb[voutid];
	struct vdspdrv_osd *osd = vdspdrv_osd[voutid];
	int i;

	DEBUG_MSG_FB ("[ipc] ambfb_activate (%d): %08x %08x\n",
			voutid, boss_fb, osd);

	/*
	 * Refresh IPC data and check if vdspdrv is active.
	 */
	if (vdspdrv_refresh() < 0)
		return -EIO;
	else if (!vdspdrv->active)
		return -ENODEV;

	if (boss_fb->active)
		return -EAGAIN;

	if (vdspdrv_refresh_osd(voutid) < 0)
		return -EIO;

	/* Take over OSD from remote OS */
	ipc_report_fb_owned();

	/* Back up OSD settings */
	memcpy(&boss_fb->original, osd, sizeof(*osd));
	memcpy(&boss_fb->bossed, osd, sizeof(*osd));

	/* Back up video plane, osd plane, and clut settings */
	boss_fb->video_plane = vdspdrv_video_plane_enabled(voutid);
	boss_fb->clut = vdspdrv_osd_get_clut(voutid);

	/* Force RGB565 mode */
	boss_fb->bossed.colorformat = VDSPDRV_COLOR_RGB565;
	boss_fb->bossed.bitsperpixel = 16;
	boss_fb->bossed.zbuf0 = (void *) ((u32) boss_fb->smem_start_virt +
			boss_fb->smem_index * boss_fb->framesize);
	boss_fb->bossed.width = boss_fb->width;
	boss_fb->bossed.height = boss_fb->height;
	boss_fb->bossed.pitch = boss_fb->pitch;
	boss_fb->bossed.rescaler_en = 0;//1;
	boss_fb->bossed.input_width = 0;//boss_fb->width;
	boss_fb->bossed.input_height = 0;//boss_fb->height;
	boss_fb->bossed.csc_en = boss_fb->csc_en;
	for (i = 0; i < 9; i++) {
		boss_fb->bossed.csc_parms[i] = boss_fb->csc_parms[i];
	}

	vdspdrv_osd_apply(voutid, &boss_fb->bossed);
	boss_fb->active = 1;

	DEBUG_MSG_FB ("[ipc] ambfb_activate (%d) END\n", voutid);

	return 0;
}

/*
 * Deactivate our frame buffer and restore pre-applied settings to DSP.
 */
static int ambfb_deactivate(int voutid)
{
	struct boss_fb *boss_fb = &G_boss_fb[voutid];

	DEBUG_MSG_FB ("[ipc] ambfb_deactivate (%d)\n", voutid);

	if (!boss_fb->active)
		return -EAGAIN;

	boss_fb->active = 0;

	/* Restore OSD settings */
	vdspdrv_osd_apply(voutid, &boss_fb->original);

	/* Hand back OSD to remote OS */
	ipc_report_fb_released();

	DEBUG_MSG_FB ("[ipc] ambfb_deactivate (%d) END\n", voutid);

	return 0;
}

#if defined(CONFIG_PROC_FS)

extern struct proc_dir_entry *proc_aipc;

struct proc_fb
{
	struct proc_dir_entry *dir;
	struct proc_dir_entry *status;
	struct proc_dir_entry *control;
};

static struct proc_fb proc_fb[2];

static int boss_fb_status_proc_read(char *page, char **start, off_t off,
				    int count, int *eof, void *data)
{
	int len = 0;
	int id = (int) data;
	struct boss_fb *boss_fb = &G_boss_fb[id];

	len += snprintf(page + len, PAGE_SIZE - len,
			"fb%d:\n", id);
	len += snprintf(page + len, PAGE_SIZE - len,
			"%d x %d\n",
			boss_fb->width, boss_fb->height);
	len += snprintf(page + len, PAGE_SIZE - len,
			"pitch: %d\n", boss_fb->pitch);
	len += snprintf(page + len, PAGE_SIZE - len,
			"active: %d\n", boss_fb->active);

	*eof = 1;

	return len;
}

static int boss_fb_control_proc_write(struct file *file,
				      const char __user *buffer,
				      unsigned long count, void *data)
{
	int id = (int) data;
	char *buf;
	int val;

	buf = kmalloc(GFP_KERNEL, count);
	if (buf == NULL)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) == 1) {
		if (val)
			ambfb_activate(id);
		else
			ambfb_deactivate(id);
	}

	kfree(buf);

	return count;
}

static void boss_fb_procfs_init(int voutid)
{
	struct proc_fb *fb;
	char tmp[128];

	fb = &proc_fb[voutid];
	snprintf(tmp, sizeof(tmp), "fb%d", voutid);
	fb->dir = proc_mkdir(tmp, proc_aipc);
	if (fb->dir == NULL)
		return;

	fb->status = create_proc_entry("status", 0, fb->dir);
	if (fb->status) {
		fb->status->data = (void *) voutid;
		fb->status->read_proc = boss_fb_status_proc_read;
		fb->status->write_proc = NULL;
	}

	fb->control = create_proc_entry("control", 0, fb->dir);
	if (fb->control) {
		fb->control->data = (void *) voutid;
		fb->control->read_proc = NULL;
		fb->control->write_proc = boss_fb_control_proc_write;
	}
}

static void boss_fb_procfs_cleanup(int voutid)
{
	struct proc_fb *fb;
	char tmp[128];

	fb = &proc_fb[voutid];
	if (fb->status) {
		remove_proc_entry("status", fb->dir);
		fb->status = NULL;
	}

	if (fb->control) {
		remove_proc_entry("control", fb->dir);
	}

	if (fb->dir) {
		snprintf(tmp, sizeof(tmp), "fb%d", voutid);
		remove_proc_entry(tmp, proc_aipc);
		fb->dir = NULL;
	}
}

#endif  /* CONFIG_PROC_FS */

/*
 * Take over frame buffer control from remote OS.
 */
static int boss_fbreq_handler_takeover(int id)
{
	DEBUG_MSG_FB ("[ipc] boss_fbreq_handler_takeover (%d)\n", id);

	ambfb_activate(id);

	DEBUG_MSG_FB ("[ipc] boss_fbreq_handler_takeover END\n");

	return 0;
}

/*
 * Release frame buffer control to remote OS.
 */
static int boss_fbreq_handler_release(int id)
{
	DEBUG_MSG_FB ("[ipc] boss_fbreq_handler_release (%d)\n", id);

	ambfb_deactivate(id);

	DEBUG_MSG_FB ("[ipc] boss_fbreq_handler_release END\n");

	return 0;
}

/*
 * Install handlers for IPC frame-buffer takeover and release.
 */
static void boss_install_fbreq_handler(void)
{
	unsigned long flags;

	local_irq_save(flags);
	fbreq_handler.takeover_req = boss_fbreq_handler_takeover;
	fbreq_handler.release_req = boss_fbreq_handler_release;
	local_irq_restore(flags);
}

/*
 * Remove handlers for IPC frame-buffer takeover and release.
 */
static void boss_uninstall_fbreq_handler(void)
{
	unsigned long flags;

	local_irq_save(flags);
	if (fbreq_handler.takeover_req == boss_fbreq_handler_takeover)
		fbreq_handler.takeover_req = NULL;
	if (fbreq_handler.release_req == boss_fbreq_handler_release)
		fbreq_handler.release_req = NULL;
	local_irq_restore(flags);
}

/*
 * Claim vdspdrv/osd resources.
 */
int ambfb_vdspdrv_claim(struct ambarella_platform_fb *pfb,
			int voutid)
{
	int rval = 0;
	struct boss_fb *boss_fb = &G_boss_fb[voutid];
	unsigned int bytesperpixel;
	unsigned int framesize, total_size;
	int colorformat;
	int fb_num = DEFAULT_FB_NUM;

	DEBUG_MSG_FB ("[ipc] ambfb_vdspdrv_claim (%d)\n", voutid);

	if (voutid == 0)
		boss_install_fbreq_handler();

	/*
	 * Refresh IPC data and check if vdspdrv is active.
	 */
	if (vdspdrv_refresh() < 0)
		return -EIO;
	else if (!vdspdrv->active)
		return -ENODEV;

	if (vdspdrv_refresh_osd(voutid) < 0)
		return -EIO;

	/*
	 * Get the exclusive video memory that is reserved for Linux
	 * from the IPC.
	 */
	if (G_boss_video_mem.mem == NULL) {
		rval = ipc_get_exfb(&G_boss_video_mem.mem,
				    &G_boss_video_mem.size);
		if (rval < 0)
			return -EIO;

		DEBUG_MSG_FB ("[ipc] G_boss_video_mem: %08x %08x (%d)\n",
			G_boss_video_mem.mem, G_boss_video_mem.size, 
			G_boss_video_mem.size);

		G_boss_video_mem.curmem = G_boss_video_mem.mem;
		G_boss_video_mem.leftover = G_boss_video_mem.size;
	}

	/*
	 * Extrapolate width and height.
	 */
	boss_fb->width = pfb->screen_var.xres;
	boss_fb->height = pfb->screen_var.yres;
	colorformat = AMBFB_COLOR_RGB565;
	bytesperpixel = 2;
	if ((pfb->screen_var.yres_virtual != 0) && (pfb->screen_var.yres != 0)) {
		fb_num = pfb->screen_var.yres_virtual / pfb->screen_var.yres;
	}

	boss_fb->pitch = boss_fb->width * bytesperpixel;
	boss_fb->csc_en = 2;
   	boss_fb->csc_parms[0] = 0x003f0275;
	boss_fb->csc_parms[1] = 0x1ea600bb;
	boss_fb->csc_parms[2] = 0x1f9901c2;
	boss_fb->csc_parms[3] = 0x1fd71e67;
	boss_fb->csc_parms[4] = 0x001001c2;
	boss_fb->csc_parms[5] = 0x00800080;
	boss_fb->csc_parms[6] = 0x00eb0010;
	boss_fb->csc_parms[7] = 0x00eb0010;
	boss_fb->csc_parms[8] = 0x00eb0010;

	framesize = boss_fb->pitch * boss_fb->height;
	boss_fb->framesize = framesize;
	if (framesize % PAGE_SIZE) {
		framesize /= PAGE_SIZE;
		framesize++;
		framesize *= PAGE_SIZE;
	}
	total_size = framesize * (fb_num + CCF_NUM);
	DEBUG_MSG_FB ("[ipc] osd: %dx%d (%d), %d, %08x\n", 
			boss_fb->width, boss_fb->height, boss_fb->pitch, 
			fb_num, total_size);

	/*
	 * Allocate and partition video memory.
	 */
	if (G_boss_video_mem.leftover >= total_size) {
		int i;

		boss_fb->smem_start_virt = G_boss_video_mem.curmem;
		boss_fb->smem_start =
			(void *) ipc_virt_to_phys((u32) boss_fb->smem_start_virt);
		boss_fb->smem_len = framesize * fb_num;
		G_boss_video_mem.curmem += boss_fb->smem_len;
		G_boss_video_mem.leftover -= boss_fb->smem_len;

		DEBUG_MSG_FB ("[ipc] fb smem: %08x (%08x) %08x\n",
			boss_fb->smem_start_virt, boss_fb->smem_start,
			boss_fb->smem_len);

		boss_fb->ccfi = 0;
		boss_fb->nccf = CCF_NUM;
		for (i = 0; i < boss_fb->nccf; i++) {
			boss_fb->ccf[i] = G_boss_video_mem.curmem;
			G_boss_video_mem.curmem += framesize;
			G_boss_video_mem.leftover -= framesize;
			memset (boss_fb->ccf[i], 0, framesize);
		}

	} else {
		DEBUG_MSG_FB ("[ipc] fbmem is not enough.. %d < %d\n",
			G_boss_video_mem.leftover, total_size);
		return -ENOMEM;
	}

	/*
	 * Set up FB subsystem parameters.
	 */
	pfb->use_prealloc = 1;
	pfb->prealloc_line_length = boss_fb->pitch;
	pfb->color_format = colorformat;
	pfb->screen_var.bits_per_pixel = bytesperpixel * 8;
	pfb->screen_var.xres = boss_fb->width;
	pfb->screen_var.xres_virtual = boss_fb->width;
	pfb->screen_var.yres = boss_fb->height;
	pfb->screen_var.yres_virtual = boss_fb->height * fb_num;
	pfb->screen_fix.smem_start = (u32) boss_fb->smem_start;
	pfb->screen_fix.smem_len = boss_fb->smem_len;
	pfb->screen_fix.line_length = boss_fb->pitch;

	/* Install and override fb_ops */
	pfb->setcmap = ambfb_vdspdrv_setcmap;
	pfb->pan_display = ambfb_vdspdrv_pan_display;

#if defined(CONFIG_PROC_FS)
	boss_fb_procfs_init(voutid);
#endif

	return 0;
}

/*
 * Release and restore vdspdrv/osd resources.
 */
int ambfb_vdspdrv_release(struct ambarella_platform_fb *pfb,
			  int voutid)
{
	DEBUG_MSG_FB ("[ipc] ambfb_vdspdrv_release (%d)\n", voutid);

	if (voutid == 0)
		boss_uninstall_fbreq_handler();

#if defined(CONFIG_PROC_FS)
	boss_fb_procfs_cleanup(voutid);
#endif

	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charles Chiou <cchiou@ambarella.com>");
MODULE_DESCRIPTION("Ambarella virtual frame buffer driver");

#endif  /* CONFIG_AMBARELLA_IPC */
