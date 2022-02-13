/*
 * boardid.c
 *
 * Copyright (C) 2012-2015 Amazon Technologies, Inc. All rights reserved.
 *
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/system_info.h>
#include <boardid.h>
#include <linux/fs.h>
#include <linux/of.h>


#define DRIVER_VER "2.0"
#define DRIVER_INFO "Board ID and Serial Number driver for Lab126 boards version " DRIVER_VER

#define PROC_IDME_DIRNAME	    	"idme"
#define BOARDID_USID_PROCNAME		"serial"
#define BOARDID_FSN_PROCNAME		"fsn"
#define BOARDID_PROCNAME_BOARDID	"board_id"
#define BOARDID_PROCNAME_PANELID	"panel_id"
#define BOARDID_PROCNAME_PCBSN		"pcbsn"
#define BOARDID_PROCNAME_MACADDR	"mac_addr"
#define BOARDID_PROCNAME_MACSEC		"mac_sec"
#define BOARDID_PROCNAME_BOOTMODE	"bootmode"
#define BOARDID_PROCNAME_POSTMODE	"postmode"
#define BOARDID_PROCNAME_BTMACADDR	"bt_mac_addr"
//#ifdef CONFIG_FALCON
#define BOARDID_PROCNAME_OLDBOOT	"oldboot"
#define BOARDID_PROCNAME_QBCOUNT	"qbcount"
//#endif
#define SYSTEM_USER_UID 1000

#define BOARDID_PROCNAME_VCOM	"vcom"
#define BOARDID_PROCNAME_MFGDATE	"mfgdate"

#define BOARDID_PROCNAME_FOS_FLAGS	"fos_flags"
#define BOARDID_PROCNAME_DEV_FLAGS	"dev_flags"
#define BOARDID_PROCNAME_USR_FLAGS	"usr_flags"

#define SERIAL_NUM_SIZE         16
#define FSN_NUM_SIZE            13
#define BOARD_ID_SIZE           16
#define PANEL_ID_SIZE           32
#define MFG_DATE_SIZE           8
#define FLAGS_SIZE              20

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif


char lab126_serial_number[SERIAL_NUM_SIZE + 1];
EXPORT_SYMBOL(lab126_serial_number);

char lab126_fsn_number[FSN_NUM_SIZE + 1];
EXPORT_SYMBOL(lab126_fsn_number);

char lab126_board_id[BOARD_ID_SIZE + 1];
EXPORT_SYMBOL(lab126_board_id);

char lab126_panel_id[PANEL_ID_SIZE + 1];
EXPORT_SYMBOL(lab126_panel_id);

char lab126_mac_address[MAC_ADDR_SIZE + 1];
EXPORT_SYMBOL(lab126_mac_address);

char lab126_btmac_address[MAC_ADDR_SIZE + 1];
EXPORT_SYMBOL(lab126_btmac_address);

char lab126_mac_secret[MAC_SEC_SIZE + 1];
EXPORT_SYMBOL(lab126_mac_secret);

char lab126_bootmode[BOOTMODE_SIZE + 1];
char lab126_postmode[BOOTMODE_SIZE + 1];

char lab126_oldboot[BOOTMODE_SIZE + 1];
char lab126_qbcount[QBCOUNT_SIZE + 1];


char lab126_vcom[VCOM_SIZE + 1];
EXPORT_SYMBOL(lab126_vcom);

char lab126_mfg_date[MFG_DATE_SIZE + 1];
EXPORT_SYMBOL(lab126_mfg_date);


char lab126_fos_flags[FLAGS_SIZE + 1];
EXPORT_SYMBOL(lab126_fos_flags);

char lab126_dev_flags[FLAGS_SIZE + 1];
EXPORT_SYMBOL(lab126_dev_flags);

char lab126_usr_flags[FLAGS_SIZE + 1];
EXPORT_SYMBOL(lab126_usr_flags);


int lab126_board_is(char *id) {
    return (BOARD_IS_(lab126_board_id, id, strlen(id)));
}
EXPORT_SYMBOL(lab126_board_is);

int lab126_board_rev_greater(char *id)
{
  return (BOARD_REV_GREATER(lab126_board_id, id));
}
EXPORT_SYMBOL(lab126_board_rev_greater);

int lab126_board_rev_greater_eq(char *id)
{
  return (BOARD_REV_GREATER_EQ(lab126_board_id, id));
}
EXPORT_SYMBOL(lab126_board_rev_greater_eq);

int lab126_board_rev_eq(char *id)
{
  return (BOARD_REV_EQ(lab126_board_id, id));
}
EXPORT_SYMBOL(lab126_board_rev_eq);

#define PCBSN_X_INDEX 5
char lab126_pcbsn_x(void)
{
  return lab126_board_id[PCBSN_X_INDEX];
}
EXPORT_SYMBOL(lab126_pcbsn_x);
	
static ssize_t proc_id_read(struct file *file, char __user *buf,size_t count, loff_t *off, char *id)
{
	ssize_t written;
	
	written = simple_read_from_buffer(buf, count, off, id, strlen(id));

	return written;
}

#define PROC_ID_READ(id) proc_id_read(file, buf, count, off,id)

static ssize_t proc_usid_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_serial_number);
}

static ssize_t proc_fsn_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_fsn_number);
}

static ssize_t proc_board_id_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_board_id);
}

static ssize_t proc_panel_id_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_panel_id);
}

static ssize_t proc_mac_address_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_mac_address);
}

static ssize_t proc_btmac_address_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_btmac_address);
}

static ssize_t proc_mac_secret_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_mac_secret);
}

static int proc_bootmode_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_bootmode);
}

static int proc_postmode_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_postmode);
}


static ssize_t proc_oldboot_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_oldboot);
}

static int proc_qbcount_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_qbcount);
}



static ssize_t proc_vcom_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_vcom);
}

static ssize_t proc_mfgdate_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_mfg_date);
}


static ssize_t proc_fos_flags_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_fos_flags);
}

static ssize_t proc_dev_flags_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_dev_flags);
}

static ssize_t proc_usr_flags_read(struct file *file, char __user *buf,size_t count, loff_t *off)
{
        return PROC_ID_READ(lab126_usr_flags);
}


static const struct file_operations proc_usid_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_usid_read,
 .write = NULL,
};

static const struct file_operations proc_fsn_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_fsn_read,
 .write = NULL,
};
	
static const struct file_operations proc_board_id_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_board_id_read,
 .write = NULL,
};

static const struct file_operations proc_panel_id_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_panel_id_read,
 .write = NULL,
};


static const struct file_operations proc_mac_address_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_mac_address_read,
 .write = NULL,
};

static const struct file_operations proc_btmac_address_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_btmac_address_read,
 .write = NULL,
};

static const struct file_operations proc_mac_secret_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_mac_secret_read,
 .write = NULL,
};

static const struct file_operations proc_bootmode_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_bootmode_read,
 .write = NULL,
};

static const struct file_operations proc_postmode_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_postmode_read,
 .write = NULL,
};

//#ifdef CONFIG_FALCON
static const struct file_operations proc_oldboot_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_oldboot_read,
 .write = NULL,
};


static const struct file_operations proc_qbcount_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_qbcount_read,
 .write = NULL,
};
//#endif

static const struct file_operations proc_vcom_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_vcom_read,
 .write = NULL,
};

static const struct file_operations proc_mfgdate_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_mfgdate_read,
 .write = NULL,
};


static const struct file_operations proc_fos_flags_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_fos_flags_read,
 .write = NULL,
};

static const struct file_operations proc_dev_flags_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_dev_flags_read,
 .write = NULL,
};

static const struct file_operations proc_usr_flags_fops = {
 .owner = THIS_MODULE, 
 .read  = proc_usr_flags_read,
 .write = NULL,
};


int bootmode_is_diags(void)
{
	return (strncmp(lab126_bootmode, "diags", 5) == 0);
}
EXPORT_SYMBOL(bootmode_is_diags);


int __init lab126_idme_vars_init(void)
{
	int r;
	struct device_node *node;
	unsigned char *temp;

	/* initialize the proc accessors */
	struct proc_dir_entry *idme_dir = proc_mkdir(PROC_IDME_DIRNAME, NULL);

	struct proc_dir_entry *proc_usid = proc_create(BOARDID_USID_PROCNAME, S_IRUGO, idme_dir,&proc_usid_fops);
	struct proc_dir_entry *proc_fsn = proc_create(BOARDID_FSN_PROCNAME, S_IRUGO, idme_dir,&proc_fsn_fops);
	struct proc_dir_entry *proc_board_id = proc_create(BOARDID_PROCNAME_BOARDID, S_IRUGO, idme_dir,&proc_board_id_fops);
	struct proc_dir_entry *proc_panel_id = proc_create(BOARDID_PROCNAME_PANELID, S_IRUGO, idme_dir,&proc_panel_id_fops);
	struct proc_dir_entry *proc_mac_address = proc_create(BOARDID_PROCNAME_MACADDR, S_IRUGO, idme_dir,&proc_mac_address_fops);
	struct proc_dir_entry *proc_mac_secret = proc_create(BOARDID_PROCNAME_MACSEC, S_IRUSR, idme_dir,&proc_mac_secret_fops);
	proc_set_user(proc_mac_secret, SYSTEM_USER_UID, 0);

	struct proc_dir_entry *proc_btmac_address = proc_create(BOARDID_PROCNAME_BTMACADDR, S_IRUGO, idme_dir,&proc_btmac_address_fops);
	struct proc_dir_entry *proc_bootmode = proc_create(BOARDID_PROCNAME_BOOTMODE, S_IRUGO, idme_dir,&proc_bootmode_fops);
	struct proc_dir_entry *proc_postmode = proc_create(BOARDID_PROCNAME_POSTMODE, S_IRUGO, idme_dir,&proc_postmode_fops);

	struct proc_dir_entry *proc_oldboot = proc_create(BOARDID_PROCNAME_OLDBOOT, S_IRUGO, idme_dir,&proc_oldboot_fops);
	struct proc_dir_entry *proc_qbcount = proc_create(BOARDID_PROCNAME_QBCOUNT, S_IRUGO, idme_dir,&proc_qbcount_fops);

	struct proc_dir_entry *proc_vcom = proc_create(BOARDID_PROCNAME_VCOM, S_IRUGO, idme_dir,&proc_vcom_fops);
	struct proc_dir_entry *proc_mfgdate = proc_create(BOARDID_PROCNAME_MFGDATE, S_IRUGO, idme_dir,&proc_mfgdate_fops);

	struct proc_dir_entry *proc_fos_flags = proc_create(BOARDID_PROCNAME_FOS_FLAGS, S_IRUGO, idme_dir,&proc_fos_flags_fops);
	struct proc_dir_entry *proc_dev_flags = proc_create(BOARDID_PROCNAME_DEV_FLAGS, S_IRUGO, idme_dir,&proc_dev_flags_fops);
	struct proc_dir_entry *proc_usr_flags = proc_create(BOARDID_PROCNAME_USR_FLAGS, S_IRUGO, idme_dir,&proc_usr_flags_fops);

	
	/* Initialize the idme values */
#if 0
	memcpy(lab126_serial_number, system_serial16, MIN(SERIAL_NUM_SIZE, sizeof(system_serial16)));
	lab126_serial_number[SERIAL_NUM_SIZE] = '\0';

	memcpy(lab126_board_id, system_rev16, MIN(BOARD_ID_SIZE, sizeof(system_rev16)));
	lab126_board_id[BOARD_ID_SIZE] = '\0';

	strcpy(lab126_panel_id, ""); /* start these as empty and populate later. */

	memcpy(lab126_mac_address, system_mac_addr, MIN(sizeof(lab126_mac_address)-1, sizeof(system_mac_addr))); 
	lab126_mac_address[sizeof(lab126_mac_address)-1] = '\0';

	memcpy(lab126_mac_secret, system_mac_sec, MIN(sizeof(lab126_mac_secret)-1, sizeof(system_mac_sec))); 
	lab126_mac_secret[sizeof(lab126_mac_secret)-1] = '\0';

	memcpy(lab126_btmac_address, system_btmac_addr, MIN(sizeof(lab126_btmac_address)-1, sizeof(system_btmac_addr))); 
	lab126_btmac_address[sizeof(lab126_btmac_address)-1] = '\0';

	memcpy(lab126_bootmode, system_bootmode, MIN(sizeof(lab126_bootmode)-1, sizeof(system_bootmode))); 
	lab126_bootmode[sizeof(lab126_bootmode)-1] = '\0';

	memcpy(lab126_postmode, system_postmode, MIN(sizeof(lab126_postmode)-1, sizeof(system_postmode))); 
	lab126_postmode[sizeof(lab126_postmode)-1] = '\0';

#ifdef CONFIG_FALCON
	memcpy(lab126_oldboot, system_oldboot, MIN(sizeof(lab126_oldboot)-1, sizeof(system_oldboot))); 
	lab126_oldboot[sizeof(lab126_oldboot)-1] = '\0';

	memcpy(lab126_qbcount, system_qbcount, MIN(sizeof(lab126_qbcount)-1, sizeof(system_qbcount))); 
	lab126_qbcount[sizeof(lab126_qbcount)-1] = '\0';
#endif

	memcpy(lab126_vcom, system_vcom, MIN(sizeof(lab126_vcom)-1, sizeof(system_vcom))); 
	lab126_vcom[sizeof(lab126_vcom)-1] = '\0';
#endif

	node = of_find_node_by_path("/chosen");
	if(NULL == node)
		return -1;	
	of_property_read_string(node, "serial", &temp);
	strcpy(lab126_serial_number, temp);
	lab126_serial_number[SERIAL_NUM_SIZE] = '\0';

	of_property_read_string(node, "fsn", &temp);
	strcpy(lab126_fsn_number, temp);
	lab126_fsn_number[FSN_NUM_SIZE] = '\0';
	
	of_property_read_string(node, "pcbsn", &temp);
	strcpy(lab126_board_id,temp);
	lab126_board_id[BOARD_ID_SIZE] = '\0'; 

	strcpy(lab126_panel_id, ""); /* start these as empty and populate later. */

	of_property_read_string(node, "mac", &temp); 
	strcpy(lab126_mac_address,temp);
	lab126_mac_address[sizeof(lab126_mac_address)-1] = '\0';

	of_property_read_string(node, "sec", &temp); 
	strcpy(lab126_mac_secret,temp);
	lab126_mac_secret[sizeof(lab126_mac_secret)-1] = '\0';

	of_property_read_string(node, "btmac", &temp); 
	strcpy(lab126_btmac_address,temp);
	lab126_btmac_address[sizeof(lab126_btmac_address)-1] = '\0';

	of_property_read_string(node, "bootmode", &temp); 
	strcpy(lab126_bootmode,temp);
	lab126_bootmode[sizeof(lab126_bootmode)-1] = '\0';

	of_property_read_string(node, "postmode", &temp); 
	strcpy(lab126_postmode,temp);
	lab126_postmode[sizeof(lab126_postmode)-1] = '\0';


	of_property_read_string(node, "oldboot", &temp); 
	strcpy(lab126_oldboot,temp);
	lab126_oldboot[sizeof(lab126_oldboot)-1] = '\0';

	of_property_read_string(node, "qbcount", &temp); 
	strcpy(lab126_qbcount,temp);
	lab126_qbcount[sizeof(lab126_qbcount)-1] = '\0';


	of_property_read_string(node, "vcom", &temp);
	strcpy(lab126_vcom,temp) ;
	lab126_vcom[sizeof(lab126_vcom)-1] = '\0';

	of_property_read_string(node, "mfgdate", &temp);
	strcpy(lab126_mfg_date,temp) ;
	lab126_mfg_date[sizeof(lab126_mfg_date)-1] = '\0';
	

	of_property_read_string(node, "fos_flags", &temp);
	strcpy(lab126_fos_flags,temp) ;
	lab126_fos_flags[sizeof(lab126_fos_flags)-1] = '\0';

	of_property_read_string(node, "dev_flags", &temp);
	strcpy(lab126_dev_flags,temp) ;
	lab126_dev_flags[sizeof(lab126_dev_flags)-1] = '\0';

	of_property_read_string(node, "usr_flags", &temp);
	strcpy(lab126_usr_flags,temp) ;
	lab126_usr_flags[sizeof(lab126_usr_flags)-1] = '\0';


	printk ("LAB126 Board id - %s\n", lab126_board_id);

	return 0;
}

EXPORT_SYMBOL(lab126_idme_vars_init);
/* Inits boardid if in case some kernel initialization code needs it
 * before idme is initialised;
 * right now mx6_cpu_op_init needs it for abc123-237 */
void __init early_init_lab126_board_id(void)
{
	int r;
	struct device_node *node;
	unsigned char *temp;

	node = of_find_node_by_path("/chosen");
	if(NULL == node)
		return;

	of_property_read_string(node, "pcbsn", &temp);
	strcpy(lab126_board_id,temp);
	lab126_board_id[BOARD_ID_SIZE] = '\0'; 
}
EXPORT_SYMBOL(early_init_lab126_board_id);



