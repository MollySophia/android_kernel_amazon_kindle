#ifndef __IOHW_RECORD_LOG_H
#define __IOHW_RECORD_LOG_H

#ifdef CONFIG_IOHW_RECORD
#ifdef __KERNEL__

#include <stdarg.h>
#include <linux/init.h>
#include <linux/kern_levels.h>
#include <linux/linkage.h>

void __init iohwrec_reserve_buf(void);
void __init setup_iohwrec_buf(void);
void __init setup_iohwrec_proc();

#endif	/* __KERNEL__ */
#endif	/* CONFIG_IOHW_RECORD */
#endif	/* __IOHW_RECORD_LOG_H */
