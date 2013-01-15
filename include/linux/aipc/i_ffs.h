/*
 * include/linux/aipc/i_ffs.h
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

#ifndef __AIPC_I_FFS_H__
#define __AIPC_I_FFS_H__

#ifdef __KERNEL__

#define IPC_FFS_SEEK_SET	0
#define IPC_FFS_SEEK_CUR	1
#define IPC_FFS_SEEK_END	2

extern int ipc_ffs_fopen(const char *file, const char *mode);
extern int ipc_ffs_close(int fd);
extern int ipc_ffs_fread(char *buf, unsigned int size, unsigned int nobj,
			 int fd);
extern int ipc_ffs_fwrite(const char *buf, unsigned int size,
			  unsigned int nobj, int fd);
extern int ipc_ffs_fseek(int fd, unsigned int offset, int origin);
extern int ipc_ffs_fsync(int fd);
extern u64 ipc_ffs_ftell(int fd);
extern int ipc_ffs_feof(int fd);

struct ipc_ffs_stat
{
	int rval;
	int fs_type;
	u64 fstfz;	/* file size in bytes */
	u16 fstact;	/* file last access time */
	u16 fstad;	/* last file access date */
	u8  fstautc;	/* last file access date and time UTC offset */
	u16 fstut;	/* last file update time */
	u16 fstuc;	/* last file update time[10ms] */
	u16 fstud;	/* last file update date */
	u8  fstuutc;	/* last file update date and time UTC offset */
	u16 fstct;	/* file create time */
	u16 fstcd;	/* file crreate date */
	u16 fstcc;	/* file create component time (ms) */
	u8  fstcutc;	/* file create date and time UTC offset */
	u16 fstat;	/* file attribute */
};

struct ipc_ffs_getdev
{
	int fs_type;
	u64 cls;	/* total number of clusters */
	u64 ecl;	/* number of unused clusters */
	int bps;	/* bytes per sector */
	int spc;	/* sectors per cluster, for udf, spc=1 */
	u32 cpg;	/* clusters per cluster group */
	u32 ecg;	/* number of empty cluster groups */
	int fmt;	/* format type */
/*#define FF_FMT_FAT12    0
#define FF_FMT_FAT16    1
#define FF_FMT_FAT31    2
#define FF_FMT_EXFAT    3*/
};

extern int ipc_ffs_fstat(const char *path, struct ipc_ffs_stat *stat);

extern int ipc_ffs_remove(const char *fname);
extern int ipc_ffs_mkdir(const char *dname);
extern int ipc_ffs_rmdir(const char *dname);
extern int ipc_ffs_rename(const char *srcname, const char *dstname);

#define IPC_FFS_SHORT_NAME_LEN	26
#define IPC_FFS_LONG_NAME_LEN	512
struct ipc_ffs_fsfind
{
	int rval;
	u32 vdta;	/* virtual dta for fsnext() */
	u16 Time;
	u16 Date;
	u64 FileSize;
	char Attribute;
	char FileName[IPC_FFS_SHORT_NAME_LEN];
	char LongName[IPC_FFS_LONG_NAME_LEN];
};

extern int ipc_ffs_fsfirst(const char *path, unsigned char attr,
			struct ipc_ffs_fsfind *fsfind);
extern int ipc_ffs_fsnext(struct ipc_ffs_fsfind *fsfind);


#endif  /* __KERNEL__ */

#endif

