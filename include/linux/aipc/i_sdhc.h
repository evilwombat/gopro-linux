/*
 * include/linux/aipc/i_sdhcl.h
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

#ifndef __AIPC_I_SDHC_H__
#define __AIPC_I_SDHC_H__

#ifdef __KERNEL__

#define IPC_SDHC_ERR_NONE		  0
#define IPC_SDHC_ERR_TIMEOUT		 -1
#define IPC_SDHC_ERR_BADCRC		 -2
#define IPC_SDHC_ERR_FAILED		 -3
#define IPC_SDHC_ERR_INVALID		 -4
#define IPC_SDHC_ERR_UNUSABLE		 -5
#define IPC_SDHC_ERR_ISR_TIMEOUT	-10
#define IPC_SDHC_ERR_DLINE_TIMEOUT	-11
#define IPC_SDHC_ERR_CLINE_TIMEOUT	-12
#define IPC_SDHC_ERR_NO_CARD		-13
#define IPC_SDHC_ERR_CHECK_PATTERN	-14

struct ipc_sdhc_command
{
	unsigned int opcode;
	unsigned int arg;
	unsigned int expect;
#define IPC_SDHC_EXPECT_NONE	0
#define IPC_SDHC_EXPECT_R1	1
#define IPC_SDHC_EXPECT_R1B	2
#define IPC_SDHC_EXPECT_R2	3
#define IPC_SDHC_EXPECT_R3	4
#define IPC_SDHC_EXPECT_R4	5
#define IPC_SDHC_EXPECT_R5	6
#define IPC_SDHC_EXPECT_R5B	7
#define IPC_SDHC_EXPECT_R6	8
#define IPC_SDHC_EXPECT_R7	9
	unsigned int retries;
	unsigned int to;
};

struct ipc_sdhc_data
{
	char *buf;
	unsigned int timeout_ns;
	unsigned int timeout_clks;
	unsigned int blksz;
	unsigned int blocks;
	unsigned int flags;
#define IPC_SDHC_DATA_READ         (0 << 1)	/**< Data read (from card) */
#define IPC_SDHC_DATA_WRITE        (1 << 1)	/**< Data write (to card) */
#define IPC_SDHC_DATA_STREAM       (1 << 2)	/**< Stream access (ACMD12) */
#define IPC_SDHC_DATA_1BIT_WIDTH   (0 << 3)	/**< 1 DAT line */
#define IPC_SDHC_DATA_4BIT_WIDTH   (1 << 3)	/**< 4 DAT lines */
#define IPC_SDHC_DATA_8BIT_WIDTH   (2 << 3)	/**< 8 DAT lines */
#define IPC_SDHC_DATA_HIGH_SPEED   (1 << 5)	/**< High speed mode */

};

struct ipc_sdhc_req
{
	struct ipc_sdhc_command *cmd;
	struct ipc_sdhc_command *stop;
	struct ipc_sdhc_data *data;
};

struct ipc_sdhc_reply
{
	int cmd_error;
	unsigned int cmd_resp[4];

	int data_error;
	unsigned int data_bytes_xfered;

	int stop_error;
	unsigned int stop_resp[4];
};

struct ipc_sdhc_ios
{
	unsigned int desired_clock;
	unsigned int vdd;
#define	IPC_SDHC_VDD_150	0	/**< 1.50 V */
#define	IPC_SDHC_VDD_155	1	/**< 1.55 V */
#define	IPC_SDHC_VDD_160	2	/**< 1.60 V */
#define	IPC_SDHC_VDD_165	3	/**< 1.65 V */
#define	IPC_SDHC_VDD_170	4	/**< 1.70 V */
#define	IPC_SDHC_VDD_180	5	/**< 1.80 V */
#define	IPC_SDHC_VDD_190	6	/**< 1.90 V */
#define	IPC_SDHC_VDD_200	7	/**< 2.00 V */
#define	IPC_SDHC_VDD_210	8	/**< 2.10 V */
#define	IPC_SDHC_VDD_220	9	/**< 2.20 V */
#define	IPC_SDHC_VDD_230	10	/**< 2.30 V */
#define	IPC_SDHC_VDD_240	11	/**< 2.40 V */
#define	IPC_SDHC_VDD_250	12	/**< 2.50 V */
#define	IPC_SDHC_VDD_260	13	/**< 2.60 V */
#define	IPC_SDHC_VDD_270	14	/**< 2.70 V */
#define	IPC_SDHC_VDD_280	15	/**< 2.80 V */
#define	IPC_SDHC_VDD_290	16	/**< 2.90 V */
#define	IPC_SDHC_VDD_300	17	/**< 3.00 V */
#define	IPC_SDHC_VDD_310	18	/**< 3.10 V */
#define	IPC_SDHC_VDD_320	19	/**< 3.20 V */
#define	IPC_SDHC_VDD_330	20	/**< 3.30 V */
#define	IPC_SDHC_VDD_340	21	/**< 3.40 V */
#define	IPC_SDHC_VDD_350	22	/**< 3.50 V */
#define	IPC_SDHC_VDD_360	23	/**< 3.60 V */
	unsigned int bus_mode;
	unsigned int power_mode;
};

extern int ipc_sdhc_num_host(void);
extern int ipc_sdhc_num_slot(int host);
extern int ipc_sdhc_card_in_slot(int host, int card);
extern int ipc_sdhc_has_iocard(int host, int card);
extern int ipc_sdhc_write_protect(int host, int card);
extern int ipc_sdhc_req(int host, int card,
			struct ipc_sdhc_req *req,
			struct ipc_sdhc_reply *reply);
extern int ipc_sdhc_abort(int host, int card);
extern int ipc_sdhc_set_ios(int host, int card, struct ipc_sdhc_ios *ios);
extern int ipc_sdhc_get_bus_status(int host, int card);

#define IPC_SDHC_EVENT_INSERT 0
#define IPC_SDHC_EVENT_EJECT 1

#define IPC_SDHC_TYPE_STORAGE 0
#define IPC_SDHC_TYPE_IOCARD 1

typedef void (*ipc_sdhc_event_handler)(int host, int card,
				       int event, int type, void *arg);

extern void ipc_sdhc_event_register(ipc_sdhc_event_handler h, void *arg);
extern void ipc_sdhc_event_unregister(ipc_sdhc_event_handler h);

typedef void (*ipc_sdhc_sdio_irq_handler)(int host, void *arg);

extern void ipc_sdhc_enable_sdio_irq(int host, int enable);
extern void ipc_sdhc_sdio_irq_register(ipc_sdhc_sdio_irq_handler h, void *arg);
extern void ipc_sdhc_sdio_irq_unregister(ipc_sdhc_sdio_irq_handler h);

#endif

#endif
