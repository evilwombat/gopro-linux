/*
 * include/linux/aipc/i_msg_request.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 08/09/2011
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

#ifndef __AIPC_I_MSG_REQUEST_H__
#define __AIPC_I_MSG_REQUEST_H__

#ifdef __KERNEL__

extern int ipc_msg_request (struct i_msg_request_s *arg);

#endif

#endif
