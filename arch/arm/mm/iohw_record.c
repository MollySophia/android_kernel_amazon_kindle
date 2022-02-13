/*
 * Copyright 2014-2016 Amazon.com, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mman.h>
#include <linux/nodemask.h>
#include <linux/memblock.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/sizes.h>
#include <linux/bootmem.h>

#include <asm/cp15.h>
#include <asm/cputype.h>
#include <asm/sections.h>
#include <asm/cachetype.h>
#include <asm/setup.h>
#include <asm/smp_plat.h>
#include <asm/tlb.h>
#include <asm/highmem.h>
#include <asm/system_info.h>
#include <asm/traps.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/pci.h>

#include "mm.h"

////////////////// HW TLB utils ////////////////////////
/* 
   ARMV7-A 
   - B3.3 for page table
   - B3.7.1 mem region attributes dependon B3-96 TRE SCTLR sys control register
   - Helpers VA to PA translation canbe involked on cp15 B3.12.32 c7
*/

#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/gfp.h>
#include <linux/vmalloc.h>
#include "fault.h"

#define GET_BITS(hi, lo, val) ((val<<(31-hi))>>(31-hi+lo))
#define TTBCR_N(ttbcr) GET_BITS(2, 0, ttbcr)
typedef enum {
	SECTION,
	SUPERSECTION,
	LARGEPAGE,
	SMALLPAGE,
	INVALID
} etype;

#define printkD(X...)

static etype ttbFindVa(u32 va, u32 ttb, u32 ttbcr, u32 sctlr, u32 table_bytes, u32 n, u32** theEntry, u32* theVa_start, u32* sidx, u32* tidx);
static u32 va2pa(u32* entry, etype etype, u32 va);
static u32* va2Mva(u32 va);
static void va2Entry(u32 va, u32** ttb1Entry, etype* ttb1Etype, u32* ttb1Va_start, u32** ttb0Entry, etype* ttb0Etype, u32* ttb0Va_start, u32* theSctlr, u32* theN, u32* sidx1, u32* tidx1, u32* sidx0, u32* tidx0);
static u32 read_ttbcr(void);
static u32 read_ttbr0(void);
static u32 read_ttbr1(void);
static u32 read_sctlr(void);
static u32 read_fcseidr(void);
static u32 write_fcseidr(u32 val);
bool getPaWatchedDb(u32* ptr, u32 deb,u32* ptr_pa);
static void interpret_pa(u32 entry,  etype ttbEtype, u32* pa_start, u32* pa_end);
static u32 vaEntry2Pa(u32 mva, u32 paStart, etype etype);
static u32 frameSize(etype entry_type);

bool notrace getPaWatchedDb(u32* ptr, u32 deb, u32* ptr_pa) {
	u32 *ttb1Entry, *ttb0Entry;
	etype ttb1Etype, ttb0Etype;
	u32 sctlr, n, mva, ttb1Va_start, ttb0Va_start;
	u32 sidx1, tidx1, sidx0, tidx0, res=0;
	u32 ttb1_pa_start, ttb1_pa_end, ttb0_pa_start, ttb0_pa_end;

	mva = va2Mva((u32)ptr);
	va2Entry(mva, &ttb1Entry, &ttb1Etype, &ttb1Va_start, &ttb0Entry, &ttb0Etype, &ttb0Va_start, &sctlr, &n, &sidx1, &tidx1, &sidx0, &tidx0);

	if(ttb1Etype != INVALID){
		interpret_pa(*ttb1Entry,  ttb1Etype, &ttb1_pa_start, &ttb1_pa_end);
		*ptr_pa = vaEntry2Pa(mva,ttb1_pa_start,ttb1Etype);	   
	}
	
	if(ttb0Etype != INVALID){
		interpret_pa(*ttb0Entry,  ttb0Etype, &ttb0_pa_start, &ttb0_pa_end);
		*ptr_pa = vaEntry2Pa(mva,ttb0_pa_start,ttb0Etype);
	}
	
	return res;
}
EXPORT_SYMBOL(getPaWatchedDb); 


static u32* notrace va2Mva(u32 va){
	//Fast Context Switch Extention
	//B3.2.1 B3-152 B3.13.2

	u32 fcseidr = read_fcseidr();	
	u32 pid = GET_BITS(31, 25, fcseidr);
	u32 va_31_25 = GET_BITS(31,25, (u32)va);
	u32 mva;

	if (pid == 0xb0000000 || va_31_25!=0)
		mva = (u32)va;
	else
		mva = (pid<<25) + (u32)va;

	return mva;
}

static void notrace interpret_pa(u32 entry,  etype ttbEtype, u32* pa_start, u32* pa_end){
	u32 frame_size;
	
	if(ttbEtype==SECTION || ttbEtype==SUPERSECTION){
		u32 sup = GET_BITS(18,18, entry);
		u32 frame_size;

		if(sup==0){
			//section: entry[31-20]+vma[19-0], thus section covers 1M: 2^20=0x100000M
			*pa_start = (GET_BITS(31, 20, entry))<<20;
			frame_size = 0x100000;
			*pa_end = *pa_start + frame_size;
		}
		else{
			//supersection: extended bits 39-32 tbd, entry[31-24]+vma[23-0], thus supersection covers 8M: 2^23=0x800000M
			*pa_start = (GET_BITS(31, 24, entry))<<24;
			frame_size = 0x800000;
			*pa_end = *pa_start + frame_size;
		}	
	}
	else
	{
		u32 page_type_bits =  GET_BITS(1,0,entry);
		etype page_type;

		if(page_type_bits == 1)
			page_type = LARGEPAGE;
		else
			page_type = SMALLPAGE;

		if(page_type==LARGEPAGE){
			//large page: entry[31-16]+vma[15-0], thus page covers 64KB: 2^16=1M
			*pa_start = (GET_BITS(31, 16, entry))<<16;
			frame_size = 0x10000;
			*pa_end = *pa_start + frame_size;
		}
		else
		{
			//small page: entry[31-12]+vma[11-0], thus page covers 4K: 2^12=1M
			*pa_start = (GET_BITS(31, 12, entry))<<12;
			frame_size = 0x1000;
			*pa_end = *pa_start + frame_size;
		}
	}
	printkD(KERN_ERR"%s:%d entry:0x%x ttbEtype:%d *pa_start:0x%x frame_size:%d *pa_end:0x%x\n",
		   __FUNCTION__,__LINE__, entry, ttbEtype, *pa_start, frame_size, *pa_end);
	return 0;
}


static u32 notrace frameSize(etype entry_type){
	switch(entry_type){
	case SECTION:
		return 0x100000;
	case SUPERSECTION:
		return 0x800000;
	case SMALLPAGE:
		return 0x1000;
	case LARGEPAGE:
		return 0x10000;
	default:
		return 0;
	}
}

static etype notrace ttbFindVa(u32 va, u32 ttb, u32 ttbcr, u32 sctlr, u32 table_bytes, u32 n, u32** theEntry, u32* theVa_start, u32* sidx, u32* tidx){
	u32 entry_type_bits, page_type_bits, sup;
	u32 *table_pa = GET_BITS(31, 14-n, ttb)<<(14-n);
	u32* entry_va;
	u32 entry, *pagetbl_pa;		
	u32 va_start;
	u32 idx = GET_BITS(31-n, 20, va)<<2;
	etype entry_type=INVALID;

	*sidx = idx;
	*tidx = 0;

	entry_va =  phys_to_virt( (u32)table_pa | (u32)idx );
	entry = *entry_va;
	entry_type_bits = GET_BITS(1,0,entry);
	va_start = 1024*1024*idx;

	switch (entry_type_bits){
	case 1:

		printkD(KERN_ERR"\n1st going to 2nd level va:0x%x va_start:0x%x idx:%d n:%d entry:0x%x entry_va:0x%x entry_type:%d table_bytes:%d\n", va, va_start, idx, n, entry, entry_va, entry_type, table_bytes);

		//PAGE TABLEs pointer
		pagetbl_pa = GET_BITS(31,10,entry)<<10;
		idx = GET_BITS(19,12,va)<<2;
		*tidx = idx;
		entry_va = (u32*)(phys_to_virt((u32)pagetbl_pa | (u32)idx));
		entry = *entry_va;
		page_type_bits =  GET_BITS(1,0,entry);
		if(page_type_bits == 1){
			entry_type = LARGEPAGE;
			va_start = va_start+idx*1024*64;
		}
		else if(page_type_bits >= 2){
			entry_type = SMALLPAGE;
			va_start = va_start+idx*1024*4;
		}
		else
			entry_type= INVALID;

		printkD(KERN_ERR"2nd level va:0x%x va_start:0x%x idx:%d pagetbl_pa:0x%x idx:%d entry_va:0x%x entry:0x%x entry_va:0x%x page_type:%s exec_never:%d\n", 
			va, va_start, idx, pagetbl_pa, idx, entry_va, entry, entry_va, entry_type==3 ? "SMALLPAGE" : entry_type==2 ? "LARGEPAGE" : "INVALID", entry_type_bits==3);
		break;
			
	case 2:
		//SECTION or SUPERSECTION
		printkD(KERN_ERR"va:0x%x idx:%d n:%d entry:0x%x entry_va:0x%x entry_type:%d table_bytes:%d\n", va, idx, n, entry, entry_va, entry_type, table_bytes);
		sup = GET_BITS(18,18, entry);
		if(sup==0)
			//section: entry[31-20]+vma[19-0], thus section covers 1M: 2^20=0x100000M
			entry_type = SECTION;
		else
			//supersection: extended bits 39-32 tbd, entry[31-24]+vma[23-0], thus supersection covers 8M: 2^23=0x800000M
			entry_type=SUPERSECTION;
	}

	*theEntry = entry_va;
	*theVa_start = va_start;
	return entry_type;
}


static void notrace va2Entry(u32 va, u32** ttb1Entry, etype* ttb1Etype, u32* ttb1Va_start, u32** ttb0Entry, etype* ttb0Etype, u32* ttb0Va_start, u32* theSctlr, u32* theN, u32* sidx1, u32* tidx1, u32* sidx0, u32* tidx0){
	u32 ttbcr = read_ttbcr();
	u32 n =  TTBCR_N(ttbcr);
	u32 ttbr_bytes = (16*1024)>>n;
	u32 sctlr = read_sctlr();
	u32 ttbr0 = read_ttbr0();
	u32 pd0 = GET_BITS(4,4,ttbcr);
	u32 ttbr1 = read_ttbr1();
	u32 pd1 = GET_BITS(5,5,ttbcr);
   
	*ttb1Etype = INVALID;
	*ttb0Etype = INVALID;

	if(pd1!=1 && n==0){
		*ttb1Etype = ttbFindVa(va, ttbr1, ttbcr, sctlr, 1024*16,0, ttb1Entry, ttb1Va_start, sidx1, tidx1);
	}

	if(pd0!=1){
		
		*ttb0Etype = ttbFindVa(va, ttbr0, ttbcr, sctlr, ttbr_bytes, n, ttb0Entry, ttb0Va_start, sidx0, tidx0);
	}

	*theSctlr = sctlr;
	*theN = n;
}

static u32 notrace va2pa(u32* entry, etype etype, u32 va){
	u32 pa;

	switch(etype){
	case SECTION:
		pa = (GET_BITS(31, 20, *entry))<<20 | GET_BITS(19,0, va);
		
		break;
	case SUPERSECTION:
		pa = (GET_BITS(31, 24, *entry))<<24 | GET_BITS(23,0, va);
		
		break;
	case LARGEPAGE:
		pa = (GET_BITS(31, 16, *entry))<<16 | GET_BITS(15,0, va);
		
		break;
	case SMALLPAGE:
		pa = (GET_BITS(31, 12, *entry))<<12 | GET_BITS(11,0, va);
		
		break;
	default:
		
		break;
	}

	return pa;
}

static u32 notrace vaEntry2Pa(u32 mva, u32 paStart, etype etype){
	u32 offset=0;

	switch (etype){
	case SECTION:
		//Figure B3-4
		offset=GET_BITS(19,0,mva);
		break;
	case SUPERSECTION:
		//Figure B3-5
		offset=GET_BITS(23,0,mva);
		break;
	case SMALLPAGE:
		//Figure B3-6
		offset=GET_BITS(11,0,mva);
		break;
	case LARGEPAGE:
		//Figure B3-7
		offset=GET_BITS(15,0,mva);
		break;
	default:
		printk(KERN_ERR"%s:%d Error: Invalid page type pa:0x%x\n",__FUNCTION__,__LINE__,paStart);
	}

	if(paStart+offset > 0x80000000)
		printkD(KERN_ERR"%s:%d paStart:0x%x offset:%d mva:0x%x\n",
			   __FUNCTION__,__LINE__,paStart, offset, mva);

	return paStart+offset;
}

static u32 read_ttbcr(void) {
	u32 lttbcr;
//Its wrong access to TTBCR in cpu_v7_do_suspend, doing c1,c0,0
	asm(
		".align 4\n"
		"mrc     p15, 0, %0, c2, c0, 2\n"
		: "=r" (lttbcr)
		);
	return lttbcr;
}

static u32 read_ttbr0(void) {
	u32 lttbr0;
	asm volatile(
		".align 4\n"
		"mrc p15, 0, %0, c2, c0, 0\n"
		: "=r" (lttbr0)
	);
    return lttbr0;
}

static u32 read_ttbr1(void) {
	u32 lttbr1;
	asm volatile(
		".align 4\n"
		"mrc p15, 0, %0, c2, c0, 1\n"
		: "=r" (lttbr1)
	);
	return lttbr1;
}

static u32 read_sctlr(void) {
	u32 sctlr;
	asm volatile(
		".align 4\n"
		"mrc p15, 0, %0, c1, c0, 1\n"
		: "=r" (sctlr)
	);
	printkD(KERN_ERR"sctlr:0x%x\n", sctlr);
	return sctlr;
}

static u32 read_fcseidr(void) {
	u32 fcseidr;
	asm volatile(
		".align 4\n"
		"mrc p15, 0, %0, c13, c0, 0\n"
		: "=r" (fcseidr)
	);
	return fcseidr;
}

static u32 write_fcseidr(u32 val) {
	u32 fcseidr;
	asm volatile(
		".align 4\n"
		"mcr p15, 0, %0, c13, c0, 0\n"
		"mrc p15, 0, %0, c13, c0, 0\n"
		: "=r" (fcseidr)
	);
	return fcseidr;
}

//based on __set_fiq_regs
static u32 int_off(void){
	u32 tmp = PSR_I_BIT | PSR_F_BIT;
	u32 flag;
	asm volatile(
		"mrs %[flag], cpsr  \n"
		"orr %[tmp], %[flag], #0xc0 \n"
		"msr cpsr_c, %[tmp]  \n"
		: [flag]"+r" (flag), [tmp]"+r" (tmp)
		: 
	);
	return flag;
}
static void int_on(u32 flag){
	asm volatile(
		"msr cpsr_c, %0 ; #copy it back, control field bit update. \n"
		: 
		: "r" (flag) 
	);
}

///////////////// Reserved Memory for HW access logging /////////////////
struct io_record {
	u16 js;
	u16 counter;
	u32 pa;
	u32 value;
	u32 pc;
};
static struct io_record* io_record_table = NULL;

#define IOHWREC_LOCATION       0x8eff0000      // All boards have at least 256MB memory. So this location should work for all boards.
#define IOHWBUF_BUF_SIZE       (PAGE_SIZE*10)  // This size is enough to record 1-2 seconds.
#define LOG_LINE_MAX_LEN       80
#define LOG_MAGIC_SIZE         4
#define MAX_REC       	       ((IOHWBUF_BUF_SIZE - LOG_MAGIC_SIZE) / sizeof(struct io_record))
#define PREV_LOG_SIZE          (LOG_LINE_MAX_LEN * MAX_REC)

static u32 start_time = 0;
static char* iohwrec_buf = NULL;
static unsigned long iohwrec_buf_location = 0;

static char* prev_log = NULL;
static int prev_log_len = 0;
static char* MAGIC = NULL;

static void mark_recording() {
	if (unlikely(MAGIC == NULL))
		return;

	// already marked?
	if (start_time != 0)
		return;

	start_time = jiffies;
	MAGIC[0] = 'I';
	MAGIC[1] = 'O';
	MAGIC[2] = 'H';
	MAGIC[3] = 'W';
}

static bool has_recorded_data() {
	return MAGIC!= NULL && MAGIC[0]=='I' && MAGIC[1]=='O' && MAGIC[2]=='H' && MAGIC[3]=='W';	
}

void __init iohwrec_reserve_buf() {
	int ret = -EBUSY;

	if (!iohwrec_buf_location) {
		iohwrec_buf_location = IOHWREC_LOCATION;
	}

	if (memblock_is_region_memory(iohwrec_buf_location, IOHWBUF_BUF_SIZE) 
		&& !memblock_is_region_reserved(iohwrec_buf_location, IOHWBUF_BUF_SIZE)) 
	{
		ret = memblock_reserve(iohwrec_buf_location, IOHWBUF_BUF_SIZE);
	}

	if (ret<0) {
		printk(KERN_ERR "Failed to alloc iohwrec buffer!");
		iohwrec_buf_location = 0;
	}
	else {
		printk(KERN_INFO "%s:%d iohwrec buffer placed at %08lx, size=%d\n", __FUNCTION__,__LINE__, iohwrec_buf_location, IOHWBUF_BUF_SIZE);
	}
}
EXPORT_SYMBOL(iohwrec_reserve_buf);

void __init setup_iohwrec_buf() {
	if (iohwrec_buf_location) {
		iohwrec_buf = __va(iohwrec_buf_location);

		MAGIC = iohwrec_buf;
		io_record_table = (struct io_record*)(iohwrec_buf + LOG_MAGIC_SIZE);

		printk("iohwrec will be located at physical address: %08lx, va: %08lx\n", iohwrec_buf_location, iohwrec_buf);

		// Backup previous IOHW log
		if (has_recorded_data()) {
			prev_log = alloc_bootmem_nopanic(PREV_LOG_SIZE);
			if (prev_log) {
				int i;
				int n = 0;
				char *buf = prev_log;
				for (i=0; i < MAX_REC; i++, buf+=n) {
					n = snprintf(buf, LOG_LINE_MAX_LEN, "(%05u.%02u) counter=%05u pa=%08x value=%08x pc=%08x\n", 
						io_record_table[i].js/100, io_record_table[i].js%100, io_record_table[i].counter,
						io_record_table[i].pa, io_record_table[i].value, io_record_table[i].pc);
				}
				prev_log_len = buf-prev_log;
			}
		}

		memset(iohwrec_buf, '\0', IOHWBUF_BUF_SIZE);
	}
}

///////////////////////// IOHW logging //////////////////////////////
// As these increase, so does CPU overhead.
enum {
	NO_INLINE_RECORD = 0,
	INLINE_RECORD_HW_BLOCKS,	// only record when new hardware areas are touched (mask out bottom byte of PA)
	INLINE_RECORD_HW_ALL,		// record all hardware accesses
	INLINE_RECORD_HW_SPINLOCK,	// all hardware accesses and spinlock/unlock
};
static u32 inline_record = NO_INLINE_RECORD;

static volatile u16 rec_index   = 0;
static volatile u16 rec_counter = 0;
static DEFINE_SPINLOCK(rec_lock);

#define OPS_DUMP_LOG 99

extern void wdg_prep(void);

static void notrace iohwbuf_write(u32 pa, u32 value, u32 pc) {
	static volatile unsigned int iohwrec_cpu = UINT_MAX;

	if (unlikely(io_record_table == NULL))
		return;

	unsigned long flags;
	int this_cpu;

	local_irq_save(flags);
	this_cpu = smp_processor_id();

	if (unlikely(iohwrec_cpu == this_cpu)) {
		if (!oops_in_progress && !lockdep_recursing(current)) {
			goto out_restore_irqs;
		}
	}

	lockdep_off();
	raw_spin_lock(&rec_lock);
	iohwrec_cpu = this_cpu;

	io_record_table[rec_index].counter = ++rec_counter;
	io_record_table[rec_index].js = jiffies - start_time;
	io_record_table[rec_index].pa = pa;
	io_record_table[rec_index].value = value;
	io_record_table[rec_index].pc = pc;
	if (++rec_index >= MAX_REC)
		rec_index = 0;

	iohwrec_cpu = UINT_MAX;
	raw_spin_unlock(&rec_lock);

	lockdep_on();

out_restore_irqs:
	local_irq_restore(flags);
}

static void dump_log() {
	if (unlikely(io_record_table == NULL))
		return;

	unsigned long flags;

	local_irq_save(flags);
	lockdep_off();
	raw_spin_lock(&rec_lock);

	int i;
	for (i=0; i < MAX_REC; i++) {
		printk(KERN_ERR "(%05u.%02u) counter=%05u pa=%08x value=%08x pc=%08x\n",
						io_record_table[i].js/100, io_record_table[i].js%100, io_record_table[i].counter,
						io_record_table[i].pa, io_record_table[i].value, io_record_table[i].pc);
	}

	raw_spin_unlock(&rec_lock);
	lockdep_on();
	local_irq_restore(flags);		
}

static ssize_t proc_iohwrec_read(struct file *file, char __user *data, size_t len, loff_t *off) {
	if (!prev_log || !prev_log_len)
		return 0;
	
	return simple_read_from_buffer(data, len, off, prev_log, prev_log_len);
}

static ssize_t proc_iohwrec_write(struct file *file, const char __user *data, size_t len, loff_t *ppos) {
	int value = 0;
	if (sscanf(data, "%d", &value) <= 0) {
		return -EINVAL;
	}

	if (value == OPS_DUMP_LOG) {
		dump_log();
		return len;
	}

	inline_record = value;
	if (value > 0)
		mark_recording();
	return len;
}

static struct file_operations iohwrec_fops = {
	.owner = THIS_MODULE,
	.read = proc_iohwrec_read,
	.write = proc_iohwrec_write,
};

void __init setup_iohwrec_proc() {
    struct proc_dir_entry *iohwrec_proc_entry;
    iohwrec_proc_entry = proc_create("iohwlog", S_IWUSR | S_IRUGO, NULL, &iohwrec_fops);
    if (!iohwrec_proc_entry) {
    	printk(KERN_ERR "%s:%s: proc init error\n", __FUNCTION__,__LINE__);
    }
}

void notrace iohw_record(const volatile void* va, u32 val, u32 pc) {
	static volatile int lastHWBlock = 0;
	u32 pa = 0;
	static int cnt = 0;
	static int recursion = 0;

	if (inline_record <= NO_INLINE_RECORD)
		return;

	if (recursion)
		return;

	// Make sure to reset this before returning.
	recursion = 1;

	// Spin_Lock
	if (va == 0xDCBA || va == 0xABCD) {
		if (inline_record < INLINE_RECORD_HW_SPINLOCK) {
			goto end_logging;
		}
	}
	// IOHW Access
	else {
		// Translate virtual address to physical address
		getPaWatchedDb((u32*)va, 0, &pa);

		// Filter address.
		// Regions are not covered:
		//   8000_0000 FFFF_FFFF: MMDC - DDR Controller
		// 	 00A0_2000 00A0_2FFF: L2 Cache
		// 	 00A0_0000 00A0_1FFF: ARM MP
		//   020B_C000 020B_FFFF: WDOG1
		if( !( // Skip anything NOT in these ranges
			(pa >= 0x020C000C && pa <=  0x7fffffff) ||
			(pa > 0 && pa <= 0x9c0000) ||
			(pa >= 0xa03000 && pa<= 0x02097FFF) ||
			(pa >= 0x0209BFFF && pa <=  0x020BBFFF)
			)) 
		{
			goto end_logging;
		}

		// Log All HW access
		if ((pa & ~0xFF) == lastHWBlock) {
			if (inline_record < INLINE_RECORD_HW_ALL) {
				goto end_logging;
			}
		}
		// Only Log Same HW Block access (INLINE_RECORD_HW_BLOCKS)
		else {
			lastHWBlock = pa & ~0xFF;
		}
	}

	// Logging and flushing
	iohwbuf_write(pa, val, pc);
	flush_cache_all();
	outer_flush_range(iohwrec_buf_location, iohwrec_buf_location + IOHWBUF_BUF_SIZE);

	// Make sure CPU overload doesn't starve watchdog thread
	if(cnt++>1000) {
		cnt=0;
		wdg_prep();
	}

end_logging:
	recursion = 0;
}
EXPORT_SYMBOL(iohw_record);
