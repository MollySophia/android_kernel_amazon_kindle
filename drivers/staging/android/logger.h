/* include/linux/logger.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Author: Robert Love <rlove@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_LOGGER_H
#define _LINUX_LOGGER_H

#include <linux/types.h>
#include <linux/ioctl.h>

#ifdef CONFIG_AMAZON_METRICS_LOG
#include <linux/metricslog.h>
#endif

/**
 * struct user_logger_entry_compat - defines a single entry that is given to a logger
 * @len:	The length of the payload
 * @__pad:	Two bytes of padding that appear to be required
 * @pid:	The generating process' process ID
 * @tid:	The generating process' thread ID
 * @sec:	The number of seconds that have elapsed since the Epoch
 * @nsec:	The number of nanoseconds that have elapsed since @sec
 * @msg:	The message that is to be logged
 *
 * The userspace structure for version 1 of the logger_entry ABI.
 * This structure is returned to userspace unless the caller requests
 * an upgrade to a newer ABI version.
 */
struct user_logger_entry_compat {
	__u16		len;
	__u16		__pad;
	__s32		pid;
	__s32		tid;
	__s32		sec;
	__s32		nsec;
	char		msg[0];
};

/**
 * struct logger_entry - defines a single entry that is given to a logger
 * @len:	The length of the payload
 * @hdr_size:	sizeof(struct logger_entry_v2)
 * @pid:	The generating process' process ID
 * @tid:	The generating process' thread ID
 * @sec:	The number of seconds that have elapsed since the Epoch
 * @nsec:	The number of nanoseconds that have elapsed since @sec
 * @euid:	Effective UID of logger
 * @msg:	The message that is to be logged
 *
 * The structure for version 2 of the logger_entry ABI.
 * This structure is returned to userspace if ioctl(LOGGER_SET_VERSION)
 * is called with version >= 2
 */
struct logger_entry {
	__u16		len;
	__u16		hdr_size;
	__s32		pid;
	__s32		tid;
	__s32		sec;
	__s32		nsec;
    __s32       tz;         /* timezone*/
	kuid_t		euid;
	char		msg[0];
};

/*
  SMP porting, we double the android buffer 
* and kernel buffer size for dual core
*/
#ifdef CONFIG_SMP
/* mingjian, 20101208: define buffer size based on different products {*/
#ifndef __MAIN_BUF_SIZE
#define __MAIN_BUF_SIZE 256*1024
#endif

#ifndef __EVENTS_BUF_SIZE
#define __EVENTS_BUF_SIZE 256*1024
#endif

#ifndef __RADIO_BUF_SIZE
#define __RADIO_BUF_SIZE 256*1024
#endif

#ifndef __SYSTEM_BUF_SIZE
#define __SYSTEM_BUF_SIZE 256*1024
#endif

#ifdef CONFIG_AMAZON_METRICS_LOG
#ifndef __METRICS_BUF_SIZE
#ifdef CONFIG_AMAZON_LOGD
#define __METRICS_BUF_SIZE (32*1024)
#else
#define __METRICS_BUF_SIZE (128*1024)
#endif /* CONFIG_AMAZON_LOGD */
#endif /* __METRICS_BUF_SIZE */

#ifndef __VITALS_BUF_SIZE
#define __VITALS_BUF_SIZE (16*1024)
#endif
#endif

#ifdef CONFIG_AMAZON_KLOG_CONSOLE
#ifndef __KERNEL_BUF_SIZE
#define __KERNEL_BUF_SIZE (256*1024)
#endif
#endif

#else

#ifndef __MAIN_BUF_SIZE
#define __MAIN_BUF_SIZE 256*1024
#endif

#ifndef __EVENTS_BUF_SIZE
#define __EVENTS_BUF_SIZE 256*1024 
#endif

#ifndef __RADIO_BUF_SIZE
#define __RADIO_BUF_SIZE 64*1024
#endif

#ifndef __SYSTEM_BUF_SIZE
#define __SYSTEM_BUF_SIZE 64*1024
#endif

#ifdef CONFIG_AMAZON_METRICS_LOG
#ifndef __METRICS_BUF_SIZE
#ifdef CONFIG_AMAZON_LOGD
#define __METRICS_BUF_SIZE (32*1024)
#else
#define __METRICS_BUF_SIZE (128*1024)
#endif /* CONFIG_AMAZON_LOGD */
#endif /* __METRICS_BUF_SIZE */

#ifndef __VITALS_BUF_SIZE
#define __VITALS_BUF_SIZE (16*1024)
#endif
#endif

#ifdef CONFIG_AMAZON_KLOG_CONSOLE
#ifndef __KERNEL_BUF_SIZE
#define __KERNEL_BUF_SIZE (128*1024)
#endif
#endif

#endif

#define LOGGER_LOG_RADIO	"log_radio"	/* radio-related messages */
#define LOGGER_LOG_EVENTS	"log_events"	/* system/hardware events */
#define LOGGER_LOG_SYSTEM	"log_system"	/* system/framework messages */
#define LOGGER_LOG_MAIN		"log_main"	/* everything else */
#ifdef CONFIG_AMAZON_LOG
#define LOGGER_LOG_AMAZON_MAIN "log_amazon_main"       /* private buffer for amazon signed apk */
#endif

#ifdef CONFIG_AMAZON_METRICS_LOG
#define LOGGER_LOG_METRICS	"log_metrics"	/* metrics logs */
#define LOGGER_LOG_AMAZON_VITALS "log_vitals"	/* vitals log */
#endif

#ifdef CONFIG_AMAZON_KLOG_CONSOLE
#define LOGGER_LOG_KERNEL   "log_kernel"  /* kernel message */
#endif

#define LOGGER_ENTRY_MAX_PAYLOAD	4076

#define __LOGGERIO	0xAE

#define LOGGER_GET_LOG_BUF_SIZE		_IO(__LOGGERIO, 1) /* size of log */
#define LOGGER_GET_LOG_LEN		_IO(__LOGGERIO, 2) /* used log len */
#define LOGGER_GET_NEXT_ENTRY_LEN	_IO(__LOGGERIO, 3) /* next entry len */
#define LOGGER_FLUSH_LOG		_IO(__LOGGERIO, 4) /* flush log */
#define LOGGER_GET_VERSION		_IO(__LOGGERIO, 5) /* abi version */
#define LOGGER_SET_VERSION		_IO(__LOGGERIO, 6) /* abi version */
#define LOGGER_SET_INTERVAL     _IO(__LOGGERIO, 101)    /* wake up interval */
#define LOGGER_SET_TIMER        _IO(__LOGGERIO, 102)    /* trigger timer*/
#endif /* _LINUX_LOGGER_H */
