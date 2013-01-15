/*
 * include/linux/aipc/i_dsp.h
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

#ifndef __AIPC_I_DSP_H__
#define __AIPC_I_DSP_H__

#ifdef __KERNEL__

struct vdspdrv
{
	int active;

	unsigned int initdata;
	int cmdsize;

	int opmode;
	unsigned int bufaddr;
	unsigned int bufsize;

	unsigned int maxcapcmds;
	unsigned int ncapcmds;
	unsigned int *capcmds;

	unsigned int maxhistory;
	unsigned int nhistory;
	void *history;
};

struct vdspdrv_osd
{
	int enabled;

	int interlaced;
	int colorformat;
#define VDSPDRV_COLOR_CLUT_8BPP		0
#define VDSPDRV_COLOR_YUV565		1
#define VDSPDRV_COLOR_AYUV4444		2
#define VDSPDRV_COLOR_AYUV1555		3
#define VDSPDRV_COLOR_YUV555		4
#define VDSPDRV_COLOR_RGB565		5
	int bitsperpixel;

	int flip;
#define VDSPDRV_FLIP_NONE
#define VDSPDRV_FLIP_H
#define VDSPDRV_FLIP_V

	int offset_x;
	int offset_y;
	int width;
	int height;
	int pitch;

	int repeat_field;

	int rescaler_en;
	int premultiplied;
	int input_width;
	int input_height;

	int global_blend;
	int transparent_color;
	int transparent_color_en;

	int csc_en;
	u32 csc_parms[9];

	void *zbuf0;
	void *zbuf1;
	void *zbuf2;
	void *zbuf3;
	void *osdupdptr;
};

extern struct vdspdrv *vdspdrv;
extern struct vdspdrv_osd *vdspdrv_osd[2];

extern int vdspdrv_refresh(void);
extern int vdspdrv_refresh_osd(int id);
extern int vdspdrv_put_cmd(void *dspcmd);
extern int vdspdrv_add_capcmd(unsigned int cmd);
extern int vdspdrv_del_capcmd(unsigned int cmd);
extern void *vdspdrv_recall_cmd(unsigned int cmd, int voutid);

extern int vdspdrv_takeover(void);
extern int vdspdrv_handback(void);

extern int vdspdrv_video_plane_enabled(int voutid);
extern int vdspdrv_en_video_plane(int voutid, int enable);

extern int vdspdrv_osd_suspend(void);
extern int vdspdrv_osd_resume(void);

extern void *vdspdrv_osd_get_clut(int voutid);
extern int vdspdrv_osd_set_clut(int voutid, void *clut);

extern int vdspdrv_osd_enable(int voutid, int enable);
extern int vdspdrv_osd_flip(int voutid, int flip);
extern int vdspdrv_osd_setbuf(int voutid, void *ptr);
extern int vdspdrv_osd_apply(int voutid, struct vdspdrv_osd *osd);

#endif  /* __KERNEL__ */

#endif
