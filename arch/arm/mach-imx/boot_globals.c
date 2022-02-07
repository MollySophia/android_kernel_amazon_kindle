/* 
 * Globals used to communicate between the bootloader (U-Boot) and the
 * the kernel (Linux) across boot-ups and restarts.
 * 
 * Copyright (C)2006-2008 Lab126, Inc.  All rights reserved.
 */

#include "boot_globals.h"
#include <asm/uaccess.h>
#include <linux/crc32.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/reboot.h>
#include <asm/io.h>
#include <linux/jiffies.h>

#define CRC32_RESIDUE 0xdebb20e3UL

extern int boot_milestone_write(const char *name, unsigned long name_len);

static const struct flag_cb {
	char *path;
	unsigned long (*flag_get)(void);
	void (*flag_set)(unsigned long);
} flag_proc[] = {
	{ "warm_restart_flag", get_warm_restart_flag, set_warm_restart_flag },
	{ "kernel_boot_flag", get_kernel_boot_flag, set_kernel_boot_flag },
};

static unsigned long boot_globals_start = BOOT_GLOBALS_BASE;
static boot_globals_t *boot_globals = NULL;

/* Return the CRC of the bytes buf[0..len-1]. */
static unsigned bg_crc32(unsigned char *buf, int len) {
   return ~crc32_le(~0L, buf, len);
}

static int reboot_boot_globals(struct notifier_block *self, unsigned long u, void *v)
{
    // Say we're "warm" and not (still) hardware-reset.
    //
    set_warm_restart_flag(BOOT_GLOBALS_WARM_FLAG);

    return NOTIFY_DONE;
}

static struct notifier_block reboot_boot_globals_nb = {
    .notifier_call = reboot_boot_globals,
    .priority = 0, /* do boot globals last */
};

static void install_reboot_notifier(void)
{
    register_reboot_notifier(&reboot_boot_globals_nb);
}

static int check_boot_globals(void)
{
	u32 computed_residue, checksum;
    
    if ( !boot_globals ) {
		return 0; /* failed because bootglobals not allocated */
	}

	if ( BOOT_GLOBALS_WARM_FLAG != get_warm_restart_flag() ) {
		return 0; /* failed because we did not restart warmly */
	}

	checksum = boot_globals->checksum;

	computed_residue = ~bg_crc32((u8 *)boot_globals, BOOT_GLOBALS_SIZE);
		
	if ( CRC32_RESIDUE != computed_residue )
	{
		return 0;
	}

    return 1;
}

static void save_boot_globals(void)
{
	if ( boot_globals ) {
        boot_globals->checksum = bg_crc32((u8 *)boot_globals, BOOT_GLOBALS_SIZE - sizeof (u32));
	}
}

boot_globals_t *get_boot_globals(void)
{
	return boot_globals;
}

void configure_boot_globals(void)
{
	static boot_globals_t bg;
    
    if (boot_globals_start != 0) {
		boot_globals=__arm_ioremap(boot_globals_start, BOOT_GLOBALS_SIZE, 0);
    }

    if(boot_globals) {
	/* Do nothing */
    } else {
        boot_globals = &bg;
    }
    
    if(!check_boot_globals()) {
		unsigned long kernel_boot_flag_tmp;

		/* preserve the boot_flag even in a cold start case */
		kernel_boot_flag_tmp=get_kernel_boot_flag();

        memset(boot_globals, 0, BOOT_GLOBALS_SIZE);

		/* preserve the boot_flag even in a cold start case */
		set_kernel_boot_flag(kernel_boot_flag_tmp);
        
        // Flag that we booted dirty (or just cold).
        //
        set_dirty_boot_flag(1);
    } else {
        
        // Flag that we didn't boot dirty (warm).
        //
        set_dirty_boot_flag(0);
    }

    // We're not warm until we've rebooted again cleanly, which our reboot notifier
    // callback will say.
    //
    set_warm_restart_flag(BOOT_GLOBALS_COLD_FLAG);
	install_reboot_notifier();
}

static int proc_flag_show (struct seq_file *m, void *v)
{
     const struct flag_cb *p =   m->private;
     if(p && p->flag_get)
      seq_printf(m,"%lu\n",p->flag_get());
    return 0;
}

static int proc_flag_open (struct inode *inode, struct file *file) 
{
	const struct flag_cb *p = PDE_DATA(file_inode(file));
        return single_open(file, proc_flag_show, (void *)p);
}

static int proc_flag_write (struct file *file, const char __user *buffer, 
				size_t count, loff_t *off)
{
	const struct flag_cb *p = PDE_DATA(file_inode(file));
	char buf[128];

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if(!count)
		return 0;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	if(p && p->flag_set) {
		unsigned long i;
		for(i=0;i<count;i++) {
			if(isspace(buf[i])) continue;
			if(buf[i]!='0') {
				/* was non-zero, set the flag */
				p->flag_set(1);
				return count;
			}
		}
		/* was all zeros - clear the flag */
		p->flag_set(0);
		return count;
	} else {
		return -EIO;
	}
}

static const struct file_operations proc_flag_fops = {
      .owner   = THIS_MODULE,
      .open    = proc_flag_open,
      .read    = seq_read,
      .llseek  = seq_lseek,
      .write   = proc_flag_write,
      .release = single_release,
};

int __init boot_globals_init(void)
{
	unsigned i;
	struct proc_dir_entry *dir;
	void configure_boot_globals(void);

	configure_boot_globals();

	dir=proc_mkdir("bootglobals", NULL);
	if(!dir) {
		printk(KERN_WARNING "could not create /proc/%s\n", "bootglobals");
		return 0;
	}

	for(i=0;i<(sizeof flag_proc/sizeof *flag_proc);i++) {
		struct proc_dir_entry *tmp;
		mode_t mode;
		
		mode=0;
		if(flag_proc[i].flag_get) {
			mode|=S_IRUGO;
		}
		if(flag_proc[i].flag_set) {
			mode|=S_IWUGO;
		}

		tmp=proc_create_data(flag_proc[i].path, mode, dir, &proc_flag_fops, (void*)&flag_proc[i]);
		if (!tmp)
		   printk(KERN_WARNING "boot globals: W could not create /proc/%s/%s\n", "bootglobals", flag_proc[i].path);
	}

	boot_milestone_write("Bg00", 4);
	return 1;
}
late_initcall(boot_globals_init);

// warm_restart_flag

void set_warm_restart_flag(unsigned long warm_restart_flag)
{
    get_boot_globals()->globals.warm_restart_flag = warm_restart_flag;
	save_boot_globals();
}

unsigned long get_warm_restart_flag(void)
{
    return get_boot_globals()->globals.warm_restart_flag;
}

// kernel_boot_flag

void set_kernel_boot_flag(unsigned long kernel_boot_flag)
{
    get_boot_globals()->globals.kernel_boot_flag = kernel_boot_flag;
}

unsigned long get_kernel_boot_flag(void)
{
    return get_boot_globals()->globals.kernel_boot_flag;
}

// apollo_init_display_flag

void set_apollo_init_display_flag(unsigned long apollo_init_display_flag)
{
    get_boot_globals()->globals.apollo_init_display_flag = apollo_init_display_flag;
}

unsigned long get_apollo_init_display_flag(void)
{
    return get_boot_globals()->globals.apollo_init_display_flag;
}

EXPORT_SYMBOL(set_apollo_init_display_flag);
EXPORT_SYMBOL(get_apollo_init_display_flag);

// dirty_boot_flag

void set_dirty_boot_flag(unsigned char dirty_boot_flag)
{
    get_boot_globals()->globals.dirty_boot_flag = dirty_boot_flag;
}

unsigned long get_dirty_boot_flag(void)
{
    return get_boot_globals()->globals.dirty_boot_flag;
}

// progress_bar_value

void set_progress_bar_value(int progress_bar_value)
{
    get_boot_globals()->globals.progress_bar_value = progress_bar_value;
}

int get_progress_bar_value(void)
{
    return get_boot_globals()->globals.progress_bar_value;
}

EXPORT_SYMBOL(set_progress_bar_value);
EXPORT_SYMBOL(get_progress_bar_value);

// user_boot_screen

void set_user_boot_screen(int user_boot_screen)
{
    get_boot_globals()->globals.user_boot_screen = user_boot_screen;
}

int get_user_boot_screen(void)
{
    return get_boot_globals()->globals.user_boot_screen;
}

EXPORT_SYMBOL(set_user_boot_screen);
EXPORT_SYMBOL(get_user_boot_screen);

// update_flag

void set_update_flag(unsigned long update_flag)
{
    get_boot_globals()->globals.update_flag = update_flag;
}

unsigned long get_update_flag(void)
{
    return get_boot_globals()->globals.update_flag;
}

// update_data

void set_update_data(unsigned long update_data)
{
    get_boot_globals()->globals.update_data = update_data;
}

unsigned long get_update_data(void)
{
    return get_boot_globals()->globals.update_data;
}

// scratch

void set_scratch(char *scratch)
{
    if ( scratch && (SCRATCH_SIZE >= strlen(scratch)) )
        strcpy(get_boot_globals()->globals.scratch, scratch);
}

char *get_scratch(void)
{
    return get_boot_globals()->globals.scratch;
}

// panel_id

void set_panel_id(panel_id_t *panel_id)
{
    if ( panel_id )
         memcpy(&get_boot_globals()->globals.panel_id, panel_id, sizeof(panel_id_t));
}

panel_id_t *get_panel_id(void)
{
    return &get_boot_globals()->globals.panel_id;
}

EXPORT_SYMBOL(get_panel_id);

void clear_panel_id(void)
{
    memset(&get_boot_globals()->globals.panel_id, 0, sizeof(panel_id_t));
}

void set_panel_size(unsigned char size)
{
    panel_id_t *panel_id = get_panel_id();
    panel_id->size = size;
}

unsigned char get_panel_size(void)
{
    panel_id_t *panel_id = get_panel_id();
    return ( panel_id->size );
}

// async/sync_settings

void set_async_settings(sync_async_t *async_settings)
{
    if ( async_settings )
        memcpy(&get_boot_globals()->globals.async_settings, async_settings, sizeof(sync_async_t));
}

sync_async_t *get_async_settings(void)
{
    return &get_boot_globals()->globals.async_settings;
}

void set_sync_settings(sync_async_t *sync_settings)
{
    if ( sync_settings )
        memcpy(&get_boot_globals()->globals.sync_settings, sync_settings, sizeof(sync_async_t));
}

sync_async_t *get_sync_settings(void)
{
    return &get_boot_globals()->globals.sync_settings;
}

void clear_async_sync_settings(void)
{
    memset(&get_boot_globals()->globals.async_settings, 0, sizeof(sync_async_t));
    memset(&get_boot_globals()->globals.sync_settings,  0, sizeof(sync_async_t));
}

// clear_dbs

void set_clear_dbs(int clear_dbs)
{
    get_boot_globals()->globals.clear_dbs = clear_dbs;
}

int get_clear_dbs(void)
{
    return get_boot_globals()->globals.clear_dbs;
}

// framebuffer_start

void set_framebuffer_start(unsigned long framebuffer_start)
{
    get_boot_globals()->globals.framebuffer_start = framebuffer_start;
}

unsigned long get_framebuffer_start(void)
{
    return get_boot_globals()->globals.framebuffer_start;
}

EXPORT_SYMBOL(get_framebuffer_start);

// drivemode_screen_ready

void set_drivemode_screen_ready(int drivemode_screen_ready)
{
    get_boot_globals()->globals.drivemode_screen_ready = drivemode_screen_ready;
}

int get_drivemode_screen_ready(void)
{
    return get_boot_globals()->globals.drivemode_screen_ready;
}

EXPORT_SYMBOL(set_drivemode_screen_ready);
EXPORT_SYMBOL(get_drivemode_screen_ready);

// hw_flags

void set_hw_flags(hw_flags_t *hw_flags)
{
    if ( hw_flags )
        memcpy(&get_boot_globals()->globals.hw_flags, hw_flags, sizeof(hw_flags_t));
}

hw_flags_t *get_hw_flags(void)
{
    return &get_boot_globals()->globals.hw_flags;
}

EXPORT_SYMBOL(get_hw_flags);

// framework_started, framework_running

void set_framework_started(int framework_started)
{
    get_boot_globals()->globals.framework_started = framework_started;
}

int get_framework_started(void)
{
    return get_boot_globals()->globals.framework_started;
}

void set_framework_running(int framework_running)
{
    get_boot_globals()->globals.framework_running = framework_running;
}

int get_framework_running(void)
{
    return get_boot_globals()->globals.framework_running;
}

void set_framework_stopped(int framework_stopped)
{
    get_boot_globals()->globals.framework_stopped = framework_stopped;
}

int get_framework_stopped(void)
{
    return get_boot_globals()->globals.framework_stopped;
}

EXPORT_SYMBOL(set_framework_started);
EXPORT_SYMBOL(get_framework_started);
EXPORT_SYMBOL(set_framework_running);
EXPORT_SYMBOL(get_framework_running);
EXPORT_SYMBOL(set_framework_stopped);
EXPORT_SYMBOL(get_framework_stopped);

// screen_clear

void set_screen_clear(int screen_clear)
{
    get_boot_globals()->globals.screen_clear = screen_clear;
}

int get_screen_clear(void)
{
    return get_boot_globals()->globals.screen_clear;
}

EXPORT_SYMBOL(set_screen_clear);
EXPORT_SYMBOL(get_screen_clear);

// battery_watermarks

void set_battery_watermarks(bat_level_t *battery_watermarks)
{
    if ( battery_watermarks )
        memcpy(&get_boot_globals()->globals.battery_watermarks, battery_watermarks, sizeof(bat_level_t));
}

bat_level_t *get_battery_watermarks(void)
{
    return &get_boot_globals()->globals.battery_watermarks;
}

// dev_mode

void set_dev_mode(int dev_mode)
{
    get_boot_globals()->globals.dev_mode = dev_mode;
}

int get_dev_mode(void)
{
    return get_boot_globals()->globals.dev_mode;
}

// env_script

void set_env_script(char *env_script)
{
    if ( env_script && (ENV_SCRIPT_SIZE >= strlen(env_script)) )
        strcpy(get_boot_globals()->globals.env_script, env_script);
}

char *get_env_script(void)
{
    return get_boot_globals()->globals.env_script;
}

// fb_ioctl

void set_fb_ioctl(sys_ioctl_t fb_ioctl)
{
    get_boot_globals()->globals.fb_ioctl = fb_ioctl;
}

sys_ioctl_t get_fb_ioctl(void)
{
    return get_boot_globals()->globals.fb_ioctl;
}

EXPORT_SYMBOL(set_fb_ioctl);
EXPORT_SYMBOL(get_fb_ioctl);

// fb_init_flag

void set_fb_init_flag(unsigned long fb_init_flag)
{
    get_boot_globals()->globals.fb_init_flag = fb_init_flag;
}

unsigned long get_fb_init_flag(void)
{
    return get_boot_globals()->globals.fb_init_flag;
}

EXPORT_SYMBOL(set_fb_init_flag);
EXPORT_SYMBOL(get_fb_init_flag);

// op_amp_offset, update_op_amp_offset_env

void set_update_op_amp_offset_env(int update_op_amp_offset_env)
{
    get_boot_globals()->globals.update_op_amp_offset_env = update_op_amp_offset_env;
}

int get_update_op_amp_offset_env(void)
{
    return get_boot_globals()->globals.update_op_amp_offset_env;
}

void set_op_amp_offset(int op_amp_offset)
{
    get_boot_globals()->globals.op_amp_offset = op_amp_offset;
}

int get_op_amp_offset(void)
{
    return get_boot_globals()->globals.op_amp_offset;
}

// rve_value

void set_rve_value(unsigned long rve_value)
{
    get_boot_globals()->globals.rve_value = rve_value;
}

unsigned long get_rve_value(void)
{
    return get_boot_globals()->globals.rve_value;
}

// gain_value

void set_gain_value(unsigned long gain_value)
{
    get_boot_globals()->globals.gain_value = gain_value;
}

unsigned long get_gain_value(void)
{
    return get_boot_globals()->globals.gain_value;
}

// pnlcd_flags

void set_pnlcd_flags(pnlcd_flags_t *pnlcd_flags)
{
    if ( pnlcd_flags )
        memcpy(&get_boot_globals()->globals.pnlcd_flags, pnlcd_flags, sizeof(pnlcd_flags_t));
}

pnlcd_flags_t *get_pnlcd_flags(void)
{
    return &get_boot_globals()->globals.pnlcd_flags;
}

int inc_pnlcd_event_count(void)
{
    return ++get_pnlcd_flags()->event_count;
}

int dec_pnlcd_event_count(void)
{
    int event_count = --get_pnlcd_flags()->event_count;
    
    if ( event_count < 0 )
        get_pnlcd_flags()->event_count = 0;

    return get_pnlcd_flags()->event_count;
}

EXPORT_SYMBOL(set_pnlcd_flags);
EXPORT_SYMBOL(get_pnlcd_flags);

EXPORT_SYMBOL(inc_pnlcd_event_count);
EXPORT_SYMBOL(dec_pnlcd_event_count);

// pnlcd_ioctl

void set_pnlcd_ioctl(sys_ioctl_t pnlcd_ioctl)
{
    get_boot_globals()->globals.pnlcd_ioctl = pnlcd_ioctl;
}

sys_ioctl_t get_pnlcd_ioctl(void)
{
    return get_boot_globals()->globals.pnlcd_ioctl;
}

EXPORT_SYMBOL(set_pnlcd_ioctl);
EXPORT_SYMBOL(get_pnlcd_ioctl);

// mw_flags

void set_mw_flags(mw_flags_t *mw_flags)
{
    if ( mw_flags )
        memcpy(&get_boot_globals()->globals.mw_flags, mw_flags, sizeof(mw_flags_t));
}

mw_flags_t *get_mw_flags(void)
{
    return &get_boot_globals()->globals.mw_flags;
}

// rcs_flags

void set_rcs_flags(rcs_flags_t *rcs_flags)
{
    if ( rcs_flags )
        memcpy(&get_boot_globals()->globals.rcs_flags, rcs_flags, sizeof(rcs_flags_t));
}

rcs_flags_t *get_rcs_flags(void)
{
    return &get_boot_globals()->globals.rcs_flags;
}

// fb_apply_fx

void set_fb_apply_fx(fb_apply_fx_t fb_apply_fx)
{
    get_boot_globals()->globals.fb_apply_fx = fb_apply_fx;
}

fb_apply_fx_t get_fb_apply_fx(void)
{
    return get_boot_globals()->globals.fb_apply_fx;
}

EXPORT_SYMBOL(set_fb_apply_fx);
EXPORT_SYMBOL(get_fb_apply_fx);

// drivemode_online

void set_drivemode_online(int drivemode_online)
{
    get_boot_globals()->globals.drivemode_online = drivemode_online;
}

int get_drivemode_online(void)
{
    return get_boot_globals()->globals.drivemode_online;
}

EXPORT_SYMBOL(set_drivemode_online);
EXPORT_SYMBOL(get_drivemode_online);

int boot_milestone_to_str(unsigned int bom_idx, char *str)
{
	const struct boot_milestone *bom_start, *bom;
	uint32_t start_jiff, jiff;
	unsigned int msecs;

	bom_start = &get_boot_globals()->globals.boot_milestones[0];
	bom = bom_start + bom_idx;
	start_jiff = bom_start->jiff;
	jiff = bom->jiff - start_jiff;
	if (jiff >= 0x20000000) {
		/* We assume that it doesn't take more than 1000 hours to
		 * boot. This allows us to handle the case where jiffies wraps
		 * around. */
		jiff = 1 + (MAX_JIFFY_OFFSET - start_jiff) + bom->jiff;
	}
	if (bom_idx > MAX_BOOT_MILESTONES) {
		return scnprintf(str, BOOT_MILESTONE_TO_STR_SZ, "invalid");
	}
	msecs = jiffies_to_msecs(jiff);
	return scnprintf(str, BOOT_MILESTONE_TO_STR_SZ, "%c%c%c%c %u\n",
		bom->name[0], bom->name[1], bom->name[2], bom->name[3],
		msecs);
}
