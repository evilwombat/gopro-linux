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

#ifndef __AIPC_I_DSIPLAY_H__
#define __AIPC_I_DSIPLAY_H__

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
/*#define VDSPDRV_COLOR_CLUT_8BPP		0
#define VDSPDRV_COLOR_YUV565		1
#define VDSPDRV_COLOR_AYUV4444		2
#define VDSPDRV_COLOR_AYUV1555		3
#define VDSPDRV_COLOR_YUV555		4
#define VDSPDRV_COLOR_RGB565		5*/

// The definition is for ione and 
// refers to system/include/dsp/dsp_vout.h in itron
#define	OSD_FORMAT_VYU_RGB_565		0x0
#define	OSD_FORMAT_UYV_BGR_565		0x1
#define	OSD_FORMAT_AYUV_4444		0x2
#define	OSD_FORMAT_RGBA_4444		0x3
#define	OSD_FORMAT_BGRA_4444		0x4
#define	OSD_FORMAT_ABGR_4444		0x5
#define	OSD_FORMAT_ARGB_4444		0x6
#define	OSD_FORMAT_AYUV_1555		0x7
#define	OSD_FORMAT_YUV_1555		0x8
#define	OSD_FORMAT_RGBA_5551		0x9
#define	OSD_FORMAT_BGRA_5551		0xA
#define	OSD_FORMAT_ABGR_1555		0xB
#define	OSD_FORMAT_ARGB_1555		0xC
#define	OSD_FORMAT_AYUV_8888		0x1B
#define	OSD_FORMAT_RGBA_8888		0x1C
#define	OSD_FORMAT_BGRA_8888		0x1D
#define	OSD_FORMAT_ABGR_8888		0x1E
#define	OSD_FORMAT_ARGB_8888		0x1F
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
//extern int vdspdrv_osd_setbuf(int voutid, void *ptr);
//extern int vdspdrv_osd_apply(int voutid, struct vdspdrv_osd *osd);


extern int display_osd_setbuf(int voutid, void* dst1, void* dst2, int width, int height, int pitch);
extern int display_osd_switch(int voutid, void* dst1, void* dst2);
extern int display_osd_apply(int voutid, struct vdspdrv_osd *osd);
extern int display_osd_back2itron(int voutid);

#endif  /* __KERNEL__ */

#endif
