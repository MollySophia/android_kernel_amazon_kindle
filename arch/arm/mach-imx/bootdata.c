/*
 * bootdata.c
 *
 * Copyright (C) 2006-2010 Amazon Technologies
 *
 */

#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "boot_globals.h"
#include <linux/jiffies.h>

// Templates for reading/writing numeric boot-global data.
//
typedef unsigned long (*get_boot_data_t)(void);
typedef void (*set_boot_data_t)(unsigned long boot_data);

#define PROC_GET_BOOT_DATA(f) return proc_get_boot_data(m, v, (get_boot_data_t)f)
#define PROC_SET_BOOT_DATA(f) return proc_set_boot_data(file, buf, count, off, (set_boot_data_t)f)

static int
proc_get_boot_data(struct seq_file *m, void *v,
		get_boot_data_t get_boot_data)
{
	seq_printf(m, "%08X\n", (unsigned int)(*get_boot_data)());
	return 0;
}

static int
proc_set_boot_data(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off,
    set_boot_data_t set_boot_data)
{
    char lbuf[16];

    memset(lbuf, 0, 16);

    if (copy_from_user(lbuf, buf, 8)) {
        return -EFAULT;
    }

    (*set_boot_data)(simple_strtol(lbuf, NULL, 16)); 

    return count;
}

// read/write update_flag
//
static int
proc_update_flag_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_update_flag);
}

static int proc_update_flag_open (struct inode *inode, struct file *file)
{
	        return single_open(file, proc_update_flag_show, NULL);
}


static int
proc_update_flag_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_update_flag);
}

static const struct file_operations proc_update_flag_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_update_flag_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .write   = proc_update_flag_write,
  .release = single_release, 
};

// read/write update_data
//
static int
proc_update_data_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_update_data);
}

static int proc_update_data_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_update_data_show, NULL);
}

static int
proc_update_data_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_update_data);
}

static const struct file_operations proc_update_data_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_update_data_open,
  .read     = seq_read,
  .llseek   = seq_lseek,
  .write    = proc_update_data_write,
  .release  = single_release,
};

// read/write drivemode_screen_ready
//

static int
proc_drivemode_screen_ready_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_drivemode_screen_ready);
}

static int
proc_drivemode_screen_ready_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_drivemode_screen_ready_show, NULL);
}

static int
proc_drivemode_screen_ready_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_drivemode_screen_ready);
}

static const struct file_operations proc_drivemode_screen_ready_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_drivemode_screen_ready_open,
  .read     = seq_read,
  .llseek   = seq_lseek,
  .write    = proc_drivemode_screen_ready_write,
  .release  = single_release,
};


// read/write framework_started, framework_running, framework_stopped
//

static int
proc_framework_started_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_framework_started);
}

static int
proc_framework_started_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_framework_started_show, NULL);
}

static int
proc_framework_started_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_framework_started);
}

static const struct file_operations proc_framework_started_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_framework_started_open,
  .read     = seq_read,
  .llseek   = seq_lseek,
  .write    = proc_framework_started_write,
  .release  = single_release,
};


static int
proc_framework_running_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_framework_running);
}

static int
proc_framework_running_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_framework_running_show, NULL);
}

static int
proc_framework_running_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_framework_running);
}

static const struct file_operations proc_framework_running_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_framework_running_open,
  .read     = seq_read,
  .llseek   = seq_lseek,
  .write    = proc_framework_running_write,
  .release  = single_release,
};

static int
proc_framework_stopped_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_framework_stopped);
}

static int
proc_framework_stopped_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_framework_stopped_show, NULL);
}

static int
proc_framework_stopped_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_framework_stopped);
}

static const struct file_operations proc_framework_stopped_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_framework_stopped_open,
  .read     = seq_read,
  .write    = proc_framework_stopped_write,
  .llseek   = seq_lseek,
  .release  = single_release,
};

// read/write env_script
//

static int
proc_env_script_show (struct seq_file *m, void *v)
{
    seq_printf (m,"%s", get_env_script());
    return 0;
}

static int
proc_env_script_open (struct inode *inode, struct file *file)
{
       return single_open(file, proc_env_script_show, NULL);
}

static int
proc_env_script_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *data)
{
    char lbuf[ENV_SCRIPT_SIZE];
    
    if (count > ENV_SCRIPT_SIZE) {
        return -EINVAL;
    }

    memset(lbuf, 0, count);

    if (copy_from_user(lbuf, buf, count)) {
        return -EFAULT;
    }

    lbuf[count] = '\0';
    set_env_script(lbuf);

    return count;
}

static const struct file_operations proc_env_script_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_env_script_open,
  .read    = seq_read,
  .write   = proc_env_script_write,
  .llseek  = seq_lseek,
  .release = single_release,
};

// read/write dirty_boot_flag
//

static int
proc_dirty_boot_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_dirty_boot_flag);
}

static int
proc_dirty_boot_open (struct inode *inode, struct file *file)
{
       return single_open(file, proc_dirty_boot_show, NULL);
}

static int
proc_dirty_boot_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_dirty_boot_flag);
}

static const struct file_operations proc_dirty_boot_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_dirty_boot_open,
  .read    = seq_read,
  .write   = proc_dirty_boot_write,
  .llseek  = seq_lseek,
  .release = single_release,
};

// read calibration_info
//

static int
proc_calibration_show (struct seq_file *m, void *v)
{
    seq_printf(m, "%d, %ld, %ld\n", get_op_amp_offset(), get_gain_value(), get_rve_value());
    return 0;
}

static int
proc_calibration_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_calibration_show, NULL);
}

static const struct file_operations proc_calibration_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_calibration_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .release = single_release,
};

// read/write panel_size
//
//
static int
proc_panel_size_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_panel_size);
}

static int
proc_panel_size_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_panel_size_show, NULL);
}

static int
proc_panel_size_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    PROC_SET_BOOT_DATA(set_panel_size);
}

static const struct file_operations proc_panel_size_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_panel_size_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .write   = proc_panel_size_write,
  .release = single_release,
};

// read panel params
//
static char *
get_panel_params(
    void)
{
    char *result = NULL;
    
    switch ( get_panel_size() )
    {
        case PANEL_ID_6_0_INCH_SIZE:
        default:
            result = PANEL_ID_6_0_INCH_PARAMS;
        break;
        
        case PANEL_ID_9_7_INCH_SIZE:
            result = PANEL_ID_9_7_INCH_PARAMS;
        break;
    }
    
    return ( result );
}


static int proc_panel_params_show (struct seq_file *m, void *v)
{
            seq_printf(m,"%s\n", get_panel_params());
	    return 0;
}

static int proc_panel_params_open (struct inode *inode, struct file *file)
{
	        return single_open(file, proc_panel_params_show, NULL);
}


static const struct file_operations proc_panel_params_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_panel_params_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .release = single_release,
};

// read/write mw_flags
//

static int
proc_mw_flags_show (struct seq_file *m, void *v)
{
    seq_printf(m, "%08lX\n", *(unsigned long *)get_mw_flags());
    return 0;
}

static int proc_mw_flags_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_mw_flags_show, NULL);
}

static int
proc_mw_flags_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    unsigned long raw_mw_flags;
    mw_flags_t mw_flags;
    char lbuf[16];

    memset(lbuf, 0, 16);

    if (copy_from_user(lbuf, buf, 8)) {
        return -EFAULT;
    }

    raw_mw_flags = (unsigned long)simple_strtol(lbuf, NULL, 0);
    memcpy(&mw_flags, &raw_mw_flags, sizeof(mw_flags_t));
    
    set_mw_flags(&mw_flags);

    return count;
}

static const struct file_operations proc_mw_flags_fops = {
  .owner   = THIS_MODULE,
  .open    = proc_mw_flags_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .write   = proc_mw_flags_write,
  .release = single_release,
};

// read/write rcs_flags
//

static int
proc_rcs_flags_show (struct seq_file *m, void *v)
{
    seq_printf(m, "%08lX\n", *(unsigned long *)get_rcs_flags());
    return 0;
}

static int 
proc_rcs_flags_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_rcs_flags_show, NULL);
}

static int
proc_rcs_flags_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *off)
{
    unsigned long raw_rcs_flags;
    rcs_flags_t rcs_flags;
    char lbuf[16];

    memset(lbuf, 0, 16);

    if (copy_from_user(lbuf, buf, 8)) {
        return -EFAULT;
    }

    raw_rcs_flags = (unsigned long)simple_strtol(lbuf, NULL, 0);
    memcpy(&rcs_flags, &raw_rcs_flags, sizeof(rcs_flags_t));
    
    set_rcs_flags(&rcs_flags);

    return count;
}

static const struct file_operations proc_rcs_flags_fops = {
  .owner  = THIS_MODULE,
  .open   = proc_rcs_flags_open,
  .read   = seq_read,
  .llseek = seq_lseek,
  .write  = proc_rcs_flags_write,
  .release= single_release,
};

// read/write progress_count
//

static int
proc_progress_count_show (struct seq_file *m, void *v)
{
    PROC_GET_BOOT_DATA(get_progress_bar_value);
}

static int
proc_progress_count_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_progress_count_show, NULL);
}
	
static int
proc_progress_count_write(struct file *file, const char __user *buf, size_t count, loff_t *off)
{
    PROC_SET_BOOT_DATA(set_progress_bar_value);
}

static const struct file_operations proc_progress_count_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_progress_count_open,
  .read     = seq_read,
  .llseek   = seq_lseek,
  .write    = proc_progress_count_write,
  .release  = single_release,
};

/* Current number of boot milestones. This starts initialized to 0 */
static unsigned int num_boot_milestones;

static rwlock_t boot_milestone_lock;

/** Read the boot milestones.
 *
 * @param buf		(out param) the buffer to put the boot milestones into.
 *			Must have at least PAGE_SIZE bytes.
 *
 * @return		Total number of bytes read, or a negative error code.
 */
static int boot_milestone_read (struct seq_file *m)
{
	char buf[BOOT_MILESTONE_TO_STR_SZ];
	unsigned int i;

	/* We assume that we can fit all of our output into a single page.  If
	 * that turns out not to be true, this has to be rewritten in a more
	 * complex way.
	 */
	BUILD_BUG_ON(BOOT_MILESTONE_TO_STR_SZ * MAX_BOOT_MILESTONES > PAGE_SIZE);

	read_lock(&boot_milestone_lock);
	if (num_boot_milestones > MAX_BOOT_MILESTONES) {
		printk("error: too many boot milestones!\n");
		read_unlock(&boot_milestone_lock);
		return -EINVAL;
	}
	for (i = 0; i < num_boot_milestones; i++) {
		
		boot_milestone_to_str(i, buf);
	        seq_printf(m,buf);
		
	}
	read_unlock(&boot_milestone_lock);
	return 0;
}

int boot_milestone_write(const char *name, unsigned long name_len)
{
	struct boot_milestone *bom;
	unsigned int ret, i;

	write_lock(&boot_milestone_lock);
	if (name_len != BOOT_MILESTONE_NAME_SZ) {
		printk(KERN_WARNING " W : milestone name must be exactly "
		       "%d alphanumeric bytes, not %ld\n",
		       BOOT_MILESTONE_NAME_SZ, name_len);
		ret = -EINVAL;
		goto done;
	}
	for (i = 0; i < BOOT_MILESTONE_NAME_SZ; i++) {
		if (! isalnum(name[i])) {
			printk(KERN_WARNING " W : won't accept "
				"non-alphanumeric milestone name "
				"0x%02x%02x%02x%02x\n",
				name[0], name[1], name[2], name[3]);
			ret = -EINVAL;
			goto done;
		}
	}
	if (! memcmp(name, "clrm", BOOT_MILESTONE_NAME_SZ)) {
		printk(KERN_NOTICE "clrm: cleared all boot milestones.\n");
		num_boot_milestones = 0;
		ret = 0;
		goto done;
	}
	if (num_boot_milestones >= MAX_BOOT_MILESTONES) {
		printk(KERN_WARNING " W : can't add another boot milestone. "
			"We already have %d boot milestones, and the maximum "
			"is %d\n", num_boot_milestones, MAX_BOOT_MILESTONES);
		ret = -EINVAL;
		goto done;
	}
	bom = get_boot_globals()->globals.boot_milestones + num_boot_milestones;
	num_boot_milestones++;
	memcpy(bom->name, name, BOOT_MILESTONE_NAME_SZ);
	bom->jiff = jiffies;
	ret = 0;

done:
	write_unlock(&boot_milestone_lock);
	return ret;
}

// read/write boot milestones
//

static int
proc_boot_milestone_show (struct seq_file *m, void *v)
{
	return boot_milestone_read(m);
}

static int 
proc_boot_milestone_open (struct inode *inode, struct file *file)
{
	return single_open(file, proc_boot_milestone_show, NULL);
}

static int
proc_boot_milestone_write(struct file *file, const char __user *buf,
	size_t count, loff_t *off)
{
	int ret;
	ret = boot_milestone_write(buf, count);
	return ret ? ret : BOOT_MILESTONE_NAME_SZ;
}

static const struct file_operations proc_boot_milestone_fops = {
  .owner    = THIS_MODULE,
  .open     = proc_boot_milestone_open,
  .read     = seq_read,
  .llseek   = seq_lseek,
  .write    = proc_boot_milestone_write,
  .release  = single_release,
};

// Entry/Exit
//
#define BD_PROC_MODE_PARENT    (S_IFDIR | S_IRUGO | S_IXUGO)
#define BD_PROC_MODE_CHILD_RW  (S_IWUGO | S_IRUGO)
#define BD_PROC_MODE_CHILD_R   (S_IRUGO)

#define BD_PROC_PARENT         "bd"
#define BD_PROC_UPDATE_FLAG    "update_flag"
#define BD_PROC_UPDATE_DATA    "update_data"
#define BD_PROC_DRIVEMODE      "drivemode_screen_ready"
#define BD_PROC_FRAMEWORK1     "framework_started"
#define BD_PROC_FRAMEWORK2     "framework_running"
#define BD_PROC_FRAMEWORK3     "framework_stopped"
#define BD_PROC_ENV_SCRIPT     "env_script"
#define BD_PROC_DIRTY_BOOT     "dirty_boot_flag"
#define BD_PROC_CALIBRATION    "calibration_info"
#define BD_PROC_PANEL_SIZE     "panel_size"
#define BD_PROC_PANEL_PARAMS   "panel_params"
#define BD_PROC_MW_FLAGS       "mw_flags"
#define BD_PROC_RCS_FLAGS      "rcs_flags"
#define BD_PROC_PROGRESS_COUNT "progress_count"
#define BD_PROC_BOOT_MILESTONE "boot_milestone"

static struct proc_dir_entry *proc_bd_parent      = NULL;
static struct proc_dir_entry *proc_update_flag    = NULL;
static struct proc_dir_entry *proc_update_data    = NULL;
static struct proc_dir_entry *proc_drivemode      = NULL;
static struct proc_dir_entry *proc_framework1     = NULL;
static struct proc_dir_entry *proc_framework2     = NULL;
static struct proc_dir_entry *proc_framework3     = NULL;
static struct proc_dir_entry *proc_env_script     = NULL;
static struct proc_dir_entry *proc_dirty_boot     = NULL;
static struct proc_dir_entry *proc_calibration    = NULL;
static struct proc_dir_entry *proc_panel_size     = NULL;
static struct proc_dir_entry *proc_panel_params   = NULL;
static struct proc_dir_entry *proc_mw_flags       = NULL;
static struct proc_dir_entry *proc_rcs_flags      = NULL;
static struct proc_dir_entry *proc_progress_count = NULL;
static struct proc_dir_entry *proc_boot_milestone = NULL;

static struct proc_dir_entry *create_bd_proc_entry(const char *name, mode_t mode, struct proc_dir_entry *parent,
    const struct file_operations *fops)
{
    return proc_create(name, mode, parent, fops);
}

#define remove_bd_proc_entry(name, entry, parent)   \
    do                                              \
    if ( entry )                                    \
    {                                               \
        remove_proc_entry(name, parent);            \
        entry = NULL;                               \
    }                                               \
    while ( 0 )

static void
bootdata_cleanup(
	void)
{
	if ( proc_bd_parent )
	{
	    remove_bd_proc_entry(BD_PROC_UPDATE_FLAG,    proc_update_flag,    proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_UPDATE_DATA,    proc_update_data,    proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_DRIVEMODE,      proc_drivemode,      proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_FRAMEWORK1,     proc_framework1,     proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_FRAMEWORK2,     proc_framework2,     proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_FRAMEWORK3,     proc_framework3,     proc_bd_parent);	    
	    remove_bd_proc_entry(BD_PROC_ENV_SCRIPT,     proc_env_script,     proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_DIRTY_BOOT,     proc_dirty_boot,     proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_CALIBRATION,    proc_calibration,    proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_PANEL_SIZE,     proc_panel_size,     proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_PANEL_PARAMS,   proc_panel_params,   proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_MW_FLAGS,       proc_mw_flags,       proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_RCS_FLAGS,      proc_rcs_flags,      proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_PROGRESS_COUNT, proc_progress_count, proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_BOOT_MILESTONE, proc_boot_milestone, proc_bd_parent);
	    remove_bd_proc_entry(BD_PROC_PARENT,         proc_bd_parent,      NULL);
	}
}

static int __init
bootdata_init(
	void)
{
    int result = -ENOMEM;

    // Parent:  /proc/bd.
    //
//    proc_bd_parent = create_proc_entry(BD_PROC_PARENT, BD_PROC_MODE_PARENT, NULL);
      proc_bd_parent = proc_mkdir(BD_PROC_PARENT, NULL);  
    if (proc_bd_parent)
    {
        int null_check = -1;
        
        // Child:   /proc/bd/update_flag.
        //
        proc_update_flag = create_bd_proc_entry(BD_PROC_UPDATE_FLAG, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_update_flag_fops);
        
        null_check &= (int)proc_update_flag;

        // Child:   /proc/bd/update_data.
        //
        proc_update_data = create_bd_proc_entry(BD_PROC_UPDATE_DATA, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_update_data_fops);
            
        null_check &= (int)proc_update_data;

        // Child:   /proc/bd/drivemode_screen_ready.
        //
        proc_drivemode = create_bd_proc_entry(BD_PROC_DRIVEMODE, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_drivemode_screen_ready_fops);
            
        null_check &= (int)proc_drivemode;
        
        // Child:   /proc/bd/framework_started.
        //
        proc_framework1 = create_bd_proc_entry(BD_PROC_FRAMEWORK1, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_framework_started_fops);
            
        null_check &= (int)proc_framework1;
        
        // Child:   /proc/bd/framework_running.
        //
        proc_framework2 = create_bd_proc_entry(BD_PROC_FRAMEWORK2, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_framework_running_fops);
            
        null_check &= (int)proc_framework2;
        
        // Child:   /proc/bd/framework_stopped.
        //
        proc_framework3 = create_bd_proc_entry(BD_PROC_FRAMEWORK3, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_framework_stopped_fops);
            
        null_check &= (int)proc_framework3;
        
        // Child:   /proc/bd/env_script.
        //
        proc_env_script = create_bd_proc_entry(BD_PROC_ENV_SCRIPT, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_env_script_fops);
            
        null_check &= (int)proc_env_script;

        // Child:   /proc/bd/dirty_boot_flag.
        //
        proc_dirty_boot = create_bd_proc_entry(BD_PROC_DIRTY_BOOT, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_dirty_boot_fops);

        null_check &= (int)proc_dirty_boot;
        
        // Child:   /proc/bd/calibration_info.
        //
        proc_calibration = create_bd_proc_entry(BD_PROC_CALIBRATION, BD_PROC_MODE_CHILD_R, proc_bd_parent,
            &proc_calibration_fops);
        
        null_check &= (int)proc_calibration;
        
        // Child:   /proc/bd/panel_size.
        //
        proc_panel_size = create_bd_proc_entry(BD_PROC_PANEL_SIZE, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_panel_size_fops);
        
        null_check &= (int)proc_panel_size;
        
        // Child:   /proc/bd/panel_params.
        //
        proc_panel_params = create_bd_proc_entry(BD_PROC_PANEL_PARAMS, BD_PROC_MODE_CHILD_R, proc_bd_parent,
            &proc_panel_params_fops);
        
        null_check &= (int)proc_panel_params;
        
        // Child:   /proc/bd/mw_flags.
        //
        proc_mw_flags = create_bd_proc_entry(BD_PROC_MW_FLAGS, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_mw_flags_fops);

        null_check &= (int)proc_mw_flags;

        // Child:   /proc/bd/rcs_flags.
        //
        proc_rcs_flags = create_bd_proc_entry(BD_PROC_RCS_FLAGS, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_rcs_flags_fops);
            
        null_check &= (int)proc_rcs_flags;

        // Child:   /proc/bd/progress_count.
        //
        proc_progress_count = create_bd_proc_entry(BD_PROC_PROGRESS_COUNT, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_progress_count_fops);

        null_check &= (int)proc_progress_count;

        // Child:   /proc/bd/boot_milestone.
        //
        proc_boot_milestone = create_bd_proc_entry(BD_PROC_BOOT_MILESTONE, BD_PROC_MODE_CHILD_RW, proc_bd_parent,
            &proc_boot_milestone_fops);

        null_check &= (int)proc_boot_milestone;
	num_boot_milestones = 0;
	rwlock_init(&boot_milestone_lock);

        // Success? 
        //
        if ( 0 == null_check )
            bootdata_cleanup();
        else
            result = 0;
    }

    // Done.
    //
	return ( result );
}

static void __exit
bootdata_exit(
	void)
{
    bootdata_cleanup();
}

module_init(bootdata_init);
module_exit(bootdata_exit);

EXPORT_SYMBOL(boot_milestone_write);

MODULE_DESCRIPTION("bootdata driver");
MODULE_AUTHOR("Lab126");
MODULE_LICENSE("Proprietary");
