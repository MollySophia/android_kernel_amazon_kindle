#ifndef __IOHW_RECORD_H
#define __IOHW_RECORD_H

#ifdef CONFIG_IOHW_RECORD
#ifdef __KERNEL__

void iohw_record(const volatile void* va, u32 val, u32 pc);
static inline void hw_record(const volatile void* va, u32 val){
	u32 pc;

	asm volatile ("mov %0, r15"
		: "=r" (pc) :
		);

	iohw_record(va, val, pc);
}

#endif	/* __KERNEL__ */
#endif	/* CONFIG_IOHW_RECORD */
#endif	/* __IOHW_RECORD_H */
