/*
 * board IDME driver
 *
 * Copyright (C) 2015 Amazon Inc., All Rights Reserved.
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <asm/bugs.h>
#include <linux/module.h>



#define DRIVER_VER "2.0"

/* Idme proc dir */
#define IDME_PROC_DIR       "idme"
#define IDME_MAGIC_NUMBER   "beefdeed"
#define IDME_MAX_NAME_LEN    16

#define MAC_SEC_KEY	"mac_sec"
#define MAC_SEC_OWNER	1000

#define DRIVER_INFO "Lab126 IDME driver version " DRIVER_VER
#define IDME_ATAG_SIZE   2048
#define IDME_MAX_ITEMS       50
extern unsigned char system_idme[IDME_ATAG_SIZE+1];
static unsigned char idme_item_name[IDME_MAX_ITEMS][IDME_MAX_NAME_LEN];

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#define IDME_ITEM_NEXT(curr_item) \
	curr_item = (struct item_t *)((char *)curr_item + sizeof(struct idme_desc) + curr_item->desc.size)

/* data structure definition for IDME */
struct idme_desc {
	char name[IDME_MAX_NAME_LEN];
	unsigned int size;
	unsigned int exportable;
	unsigned int permission;
};

struct item_t {
	struct idme_desc desc;
	unsigned char data[0];
};

struct idme_t {
	char magic[8];
	char version[4];
	unsigned int items_num;

	unsigned char item_data[0];
};


int idme_check_magic_number(struct idme_t *pidme)
{
	if(pidme == NULL) {
		printk(KERN_ERR "IDME: the pointer of pidme_data is NULL.\n");
		return -1;
	}

	if (strncmp(pidme->magic, IDME_MAGIC_NUMBER, strlen(IDME_MAGIC_NUMBER))){
		printk(KERN_ERR "IDME: idme data is invalid!\n");
		return -2;
	}
	else
		return 0;
}

/*#define abc123_HVT_DEVICE_ID_DEFAULT*/
/* to be removed when this field is written in factory and populated in LK */
#if defined(abc123_HVT_DEVICE_ID_DEFAULT)
static unsigned char abc123_device_type[]="A2M4YX06LWP8WI";
static unsigned char abc123_device_type_name[]="device_type_id";
#endif

int idme_get_var(const char *name, char *buf, unsigned int length)
{
	int ret = -1;
	unsigned int i = 0;
	struct idme_t *pdata = (struct idme_t *)&system_idme[0];
	struct item_t *pitem = NULL;

	if (0 != idme_check_magic_number(pdata)){
		printk(KERN_ERR "The idme magic number error.\n");
		return -1;
	}

	if (NULL == buf)
		return -1;

#if defined(abc123_HVT_DEVICE_ID_DEFAULT)
        if (strcmp(name, abc123_device_type_name) == 0)
        {
		ret = sizeof(abc123_device_type);
                printk(KERN_ERR "%s: abc123_device_type = %s[%d]\n",__func__,abc123_device_type,ret);
                memcpy(buf,abc123_device_type,ret);
		return ret;
        }
#endif

	pitem = (struct item_t *)(&(pdata->item_data[0]));
	for (i = 0; i < pdata->items_num; i++) {
		if ( 0 == strcmp(name, pitem->desc.name) ) {
			memcpy(buf, &(pitem->data[0]), MIN( pitem->desc.size, length ) );
			ret = MIN(pitem->desc.size, length);
			break;
		}else{
			IDME_ITEM_NEXT(pitem);
		}
	}

	return ret;
}



static ssize_t idme_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	char *idme_item_name;
	unsigned char idme_item_data[1028] = {'\0'};
	int  idme_item_size = 0;
	size_t size = 0;
	loff_t	pos = *ppos;

	idme_item_name = PDE_DATA(file_inode(file));
	if(!(idme_item_name)){
		printk(KERN_ERR "IDME Name is Null");
		return 0;
	}
	//printk(KERN_ERR "%s: item name is %s %zx %d\n", __func__, idme_item_name, count, (int)pos);

	idme_item_size = idme_get_var((const char*)idme_item_name, idme_item_data, sizeof(idme_item_data));
	if (pos >= idme_item_size) return 0;
	if (idme_item_size > 0) {
		size = MIN(count, idme_item_size);
		copy_to_user(buf,idme_item_data, size);
	} else {
		printk(KERN_ERR "%s: idme read error\n", __func__);
	}

	if ((ssize_t) size > 0)
		*ppos = pos + size;
	return size;
}

static const struct file_operations idme_proc_fops = {
	.read	= idme_read,
};

void create_idme_proc(void)
{
	int i = 0;
	struct proc_dir_entry *proc_item[100];
	unsigned char *pidme;
	unsigned int *scan;
	char name[32] = {'\0'};
	char idme_magic_number[9] = {'\0'};
	char idme_version[5] = {'\0'};
	unsigned int idme_item_num, exportable, permission, data_size;
	unsigned int *ptemp;
	unsigned char data[128] = {0};
	struct proc_dir_entry *idme_dir = NULL;
	bool access_restrict = false;

	pidme = &system_idme[0];
	idme_dir = proc_mkdir(IDME_PROC_DIR, NULL);

	/* check IDME magic number */
	strncpy(idme_magic_number, pidme, 8);
	if (strncmp(idme_magic_number, IDME_MAGIC_NUMBER, strlen(IDME_MAGIC_NUMBER))) {
		printk(KERN_ERR"%s: invalid IDME magic number %s\n", __func__, idme_magic_number);
		return;
	}
	pidme += 8;

	/* check IDME version */
	strncpy(idme_version, pidme, 4);
	pidme += 4;

	/* get the IDME item number */
	ptemp = (unsigned int *)pidme;
	idme_item_num = *ptemp;
	pidme += sizeof(unsigned int);
	printk(KERN_INFO"%s: IDME version %s, item size %d\n", __func__, idme_version, idme_item_num);

	for (i = 0; i < idme_item_num; i++) {
		// set get name
		strncpy(name, pidme, 16);
		pidme += 16;

		// get data size
		ptemp = (unsigned int *)pidme;
		data_size = *ptemp;
		pidme += sizeof(unsigned int);

                // get exportable
		ptemp = (unsigned int *)pidme;
		exportable = *ptemp;
		pidme += sizeof(unsigned int);

		// get permission
		ptemp = (unsigned int *)pidme;
		permission = *ptemp;
		pidme += sizeof(unsigned int);

		// get data
		//memcpy(data, pidme, data_size);
		pidme += data_size;

		/* Restrict mac_sec */
		if (strcmp(name, MAC_SEC_KEY) == 0) {
			access_restrict = true;
			permission = 0400;
		} else {
			access_restrict = false;
		}

		// set name and read its function
		if(exportable > 0) {
			strncpy(idme_item_name[i], name, strlen(name));
			proc_item[i] = proc_create_data(name, permission, idme_dir, &idme_proc_fops, idme_item_name[i]);

			if (proc_item[i] && access_restrict)
				proc_set_user(proc_item[i], MAC_SEC_OWNER, 0);
		}
	}
#if defined(abc123_HVT_DEVICE_ID_DEFAULT)
        /* add device_type_id */
	proc_create_data(abc123_device_type_name, permission, idme_dir, &idme_proc_fops, abc123_device_type_name);
#endif
	return;
}

/* copy initialize the idme */
static void initialize_idme(void)
{
	create_idme_proc();
}
        
void init_idme(void)
{
        printk(DRIVER_INFO "\n");
        initialize_idme();
}

module_init(init_idme);
