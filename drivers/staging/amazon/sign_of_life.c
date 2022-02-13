/*
 * sign_of_life.c
 *
 * Device Sign of Life information
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Yang Liu (yangliu@lab126.com)
 * TODO: Add additional contributor's names.
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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/sign_of_life.h>
#include <mach/mt_boot_common.h>

#define DEV_SOL_VERSION     "1.0"
#define DEV_SOL_PROC_NAME   "life_cycle_reason"
#define MAX_SIZE             10

static struct proc_dir_entry *life_cycle_metrics_file;

typedef enum {
	LIFE_CYCLES_DEFAULT_REASON = 0,
	SW_SHUTDOWN,
	LONG_KEY_PRESSED_PWR_KEY_SHUTDOWN,
	PMIC_THERMAL_SHUTDOWN,
	BATTERY_THERMAL_SHUTDOWN,
	SOC_THERMAL_SHUTDOWN,
	PCB_THERMAL_SHUTDOWN,
	WIFI_THERMAL_SHUTDOWN,
	MODEM_THERMAL_SHUTDOWN,
	LOW_BATTERY_SHUTDOWN,
	SUDDEN_POWER_LOSS_SHUTDOWN,
	UNKNOWN_SHUTDOWN,
	COLD_BOOT_BY_PWR_KEY,
	COLD_BOOT_BY_USB_CHARGER,
	COLD_BOOT_BY_POWER_SUPPLY,
	WARM_BOOT_BY_SW,
	WARM_BOOT_BY_KERNEL_PANIC,
	WARM_BOOT_BY_KERNEL_WDG,
	WARM_BOOT_BY_HW_WDG,
	PWR_OFF_CHARGING_MODE,
	FACTORY_RESET_REBOOT,
	OTA_REBOOT,
} life_cycle_reasons_idx;

static const char * const life_cycle_reasons[] = {
	"Life Cycle Reason Not Available",
	"Software Shutdown",
	"Long Pressed Power Key Shutdown",
	"PMIC Overheated Thermal Shutdown",
	"Battery Overheated Thermal Shutdown",
	"SOC Overheated Thermal Shutdown",
	"PCB Overheated Thermal Shutdown",
	"WIFI Overheated Thermal Shutdown",
	"Modem Overheated Thermal Shutdown",
	"Low Battery Shutdown",
	"Sudden Power Loss Shutdown",
	"Unknown Shutdown",
	"Cold Boot By Power Key",
	"Cold Boot By USB Charger",
	"Cold Boot By Power Supply",
	"Warm Boot By Software",
	"Warm Boot By Kernel Panic",
	"Warm Boot By Kernel Watchdog",
	"Warm Boot By HW Watchdog",
	"Power Off Charging Mode",
	"Factory Reset Reboot",
	"OTA Reboot",
};

static const char * const life_cycle_metrics[] = {
	"not_available",
	"sw_shutdown",
	"long_key_press_pwr_key_shutdown",
	"pmic_thermal_shutdown",
	"battery_thermal_shutdown",
	"soc_thermal_shutdown",
	"pcb_thermal_shutdown",
	"wifi_thermal_shutdown",
	"modem_thermal_shutdown",
	"low_battery_shutdown",
	"sudden_power_loss",
	"unknown_shutdown",
	"cold_boot_pwr_key",
	"cold_boot_usb_charger",
	"cold_boot_power_supply",
	"warm_boot_sw",
	"warm_boot_kernel_panic",
	"warm_boot_kernel_wdog",
	"warm_boot_hw_wdog",
	"pwr_off_charging",
	"factory_reset",
	"ota_reboot",
};

static const char * const life_cycle_vitals_source[] = {
	"LCR_abnormal",
	"LCR_normal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_normal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_normal",
	"LCR_normal",
	"LCR_normal",
	"LCR_normal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_abnormal",
	"LCR_normal",
	"LCR_normal",
	"LCR_normal",
};

static const char * const life_cycle_vitals_key[] = {
	"Unknown_Shutdown",
	"Android_Shutdown",
	"Long_Key_Press_Shutdown",
	"Thermal_Shutdown",
	"Thermal_Shutdown",
	"Thermal_Shutdown",
	"Thermal_Shutdown",
	"Thermal_Shutdown",
	"Thermal_Shutdown",
	"Critically_Low_Battery_Shutdown",
	"Sudden_Power_Loss",
	"Unknown_Shutdown",
	"Cold_Boot_By_Power_Key",
	"Cold_Boot_By_USB",
	"Cold_Boot_By_Power_Supply",
	"Warm_Boot_By_Android",
	"Warm_Boot_Kernel_Panic",
	"Warm_Boot_Kernel_Watchdog",
	"Warm_Boot_Hardware_Watchdog",
	"Power_Off_Charging",
	"Factory_Reset_Reboot",
	"OTA_Reboot",
};

static const char * const life_cycle_vitals_value2[] = {
	NULL,
	NULL,
	NULL,
	"PMIC",
	"BATTERY",
	"SOC",
	"PCB",
	"WIFI",
	"MODEM",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

struct dev_sol {
	u8   *data;
	u8   life_cycle_reason_idx;
	sign_of_life_ops  sol_ops;
	struct mutex lock;
};

static struct dev_sol *p_dev_sol;

static int life_cycle_reason_lookup(void)
{
	life_cycle_boot_reason boot_reason = 0;
	life_cycle_shutdown_reason shutdown_reason = 0;
	life_cycle_thermal_shutdown_reason thermal_shutdown_reason = 0;
	life_cycle_special_mode s_mode = 0;
	bool lcr_found = false;

	if (!p_dev_sol) {
		printk(KERN_ERR "%s: the life cycle driver is not initialized\n", __func__);
		return -1;
	}

	if (!((p_dev_sol->sol_ops.read_boot_reason) &&
		(p_dev_sol->sol_ops.read_shutdown_reason) &&
		(p_dev_sol->sol_ops.read_thermal_shutdown_reason) &&
		(p_dev_sol->sol_ops.read_special_mode))) {
		printk(KERN_ERR "%s: no platform supported\n", __func__);
		return -1;
	}

	p_dev_sol->sol_ops.read_boot_reason(&boot_reason);
	p_dev_sol->sol_ops.read_shutdown_reason(&shutdown_reason);
	p_dev_sol->sol_ops.read_thermal_shutdown_reason(&thermal_shutdown_reason);
	p_dev_sol->sol_ops.read_special_mode(&s_mode);

	/* thermal shutdown is considered as abnormal */
	switch (thermal_shutdown_reason) {
	case THERMAL_SHUTDOWN_REASON_BATTERY:
		p_dev_sol->life_cycle_reason_idx = BATTERY_THERMAL_SHUTDOWN;
		lcr_found = true;
		break;
	case THERMAL_SHUTDOWN_REASON_PMIC:
		p_dev_sol->life_cycle_reason_idx = PMIC_THERMAL_SHUTDOWN;
		lcr_found = true;
		break;
	case THERMAL_SHUTDOWN_REASON_SOC:
		p_dev_sol->life_cycle_reason_idx = SOC_THERMAL_SHUTDOWN;
		lcr_found = true;
		break;
	case THERMAL_SHUTDOWN_REASON_MODEM:
		p_dev_sol->life_cycle_reason_idx = MODEM_THERMAL_SHUTDOWN;
		lcr_found = true;
		break;
	case THERMAL_SHUTDOWN_REASON_WIFI:
		p_dev_sol->life_cycle_reason_idx = WIFI_THERMAL_SHUTDOWN;
		lcr_found = true;
		break;
	case THERMAL_SHUTDOWN_REASON_PCB:
		p_dev_sol->life_cycle_reason_idx = PCB_THERMAL_SHUTDOWN;
		lcr_found = true;
		break;
	default:
		break;
	}

	if (lcr_found)
		return 0;

	/* boot reason: abnormal */
	switch (boot_reason) {
	case WARMBOOT_BY_KERNEL_WATCHDOG:
		p_dev_sol->life_cycle_reason_idx = WARM_BOOT_BY_KERNEL_WDG;
		lcr_found = true;
		break;
	case WARMBOOT_BY_KERNEL_PANIC:
		p_dev_sol->life_cycle_reason_idx = WARM_BOOT_BY_KERNEL_PANIC;
		lcr_found = true;
		break;
	case WARMBOOT_BY_HW_WATCHDOG:
		p_dev_sol->life_cycle_reason_idx = WARM_BOOT_BY_HW_WDG;
		lcr_found = true;
		break;
	default:
		break;
	}

	/* shutdown reason: abnormal */
	switch (shutdown_reason) {
	case SHUTDOWN_BY_LONG_PWR_KEY_PRESS:
		p_dev_sol->life_cycle_reason_idx = LONG_KEY_PRESSED_PWR_KEY_SHUTDOWN;
		lcr_found = true;
		break;
	case SHUTDOWN_BY_UNKNOWN_REASONS:
		p_dev_sol->life_cycle_reason_idx = UNKNOWN_SHUTDOWN;
		lcr_found = true;
		break;
	default:
		break;
	}

	if (lcr_found)
		return 0;

	/* boot reason: normal */
	switch (boot_reason) {
	case WARMBOOT_BY_SW:
		p_dev_sol->life_cycle_reason_idx = WARM_BOOT_BY_SW;
		lcr_found = true;
		break;

	case COLDBOOT_BY_USB:
		p_dev_sol->life_cycle_reason_idx = COLD_BOOT_BY_USB_CHARGER;
		lcr_found = true;
		break;

	case COLDBOOT_BY_POWER_KEY:
		p_dev_sol->life_cycle_reason_idx = COLD_BOOT_BY_PWR_KEY;
		lcr_found = true;
		break;

	case COLDBOOT_BY_POWER_SUPPLY:
		p_dev_sol->life_cycle_reason_idx = COLD_BOOT_BY_POWER_SUPPLY;
		lcr_found = true;
		break;

	default:
		break;
	}

	if (lcr_found)
		return 0;

	/* shutdown reason: normal */
	switch (shutdown_reason) {
	case SHUTDOWN_BY_SW:
		p_dev_sol->life_cycle_reason_idx = SW_SHUTDOWN;
		lcr_found = true;
		break;
	default:
		break;
	}

	return 0;
}

static int life_cycle_metrics_show(struct seq_file *m, void *v)
{
	if (p_dev_sol) {
		seq_printf(m, "%s", life_cycle_reasons[p_dev_sol->life_cycle_reason_idx]);
	} else
		seq_printf(m, "life cycle reason driver is not initialized");
	return 0;
}

static int life_cycle_metrics_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, life_cycle_metrics_show, NULL);
}

static const struct file_operations life_cycle_metrics_proc_fops = {
	.open = life_cycle_metrics_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void life_cycle_metrics_proc_init(void)
{
	life_cycle_metrics_file = proc_create(DEV_SOL_PROC_NAME,
				  0444, NULL, &life_cycle_metrics_proc_fops);
	if (life_cycle_metrics_file == NULL) {
		printk(KERN_ERR "%s: Can't create life cycle metrics proc entry\n", __func__);
	}
}
EXPORT_SYMBOL(life_cycle_metrics_proc_init);

void life_cycle_metrics_proc_done(void)
{
	remove_proc_entry(DEV_SOL_PROC_NAME, NULL);
}
EXPORT_SYMBOL(life_cycle_metrics_proc_done);


/*
 * life_cycle_set_boot_reason
 * Description: this function will set the boot reason which causing device booting
 */
int life_cycle_set_boot_reason(life_cycle_boot_reason boot_reason)
{
	if (!p_dev_sol) {
		printk(KERN_ERR "%s: the life cycle driver is not initialized\n", __func__);
		return -1;
	}

	if (!p_dev_sol->sol_ops.write_boot_reason) {
		printk(KERN_ERR "%s: no platform supported\n", __func__);
		return -1;
	} else
		p_dev_sol->sol_ops.write_boot_reason(boot_reason);

	return 0;
}
EXPORT_SYMBOL(life_cycle_set_boot_reason);


/*
 * life_cycle_set_shutdown_reason
 * Description: this function will set the Shutdown reason which causing device shutdown
 */
int life_cycle_set_shutdown_reason(life_cycle_shutdown_reason shutdown_reason)
{
	if (!p_dev_sol) {
		printk(KERN_ERR "%s: the life cycle driver is not initialized\n", __func__);
		return -1;
	}

	if (!p_dev_sol->sol_ops.write_shutdown_reason) {
		printk(KERN_ERR "%s: no platform supported\n", __func__);
		return -1;
	} else
		p_dev_sol->sol_ops.write_shutdown_reason(shutdown_reason);

	return 0;
}
EXPORT_SYMBOL(life_cycle_set_shutdown_reason);

/*
 * life_cycle_set_thermal_shutdown_reason
 * Description: this function will set the Thermal Shutdown reason which causing device shutdown
 */
int life_cycle_set_thermal_shutdown_reason(life_cycle_thermal_shutdown_reason thermal_shutdown_reason)
{
	if (!p_dev_sol) {
		printk(KERN_ERR "%s: the life cycle driver is not initialized\n", __func__);
		return -1;
	}

	if (!p_dev_sol->sol_ops.write_thermal_shutdown_reason) {
		printk(KERN_ERR "%s: no platform supported\n", __func__);
		return -1;
	} else
		p_dev_sol->sol_ops.write_thermal_shutdown_reason(thermal_shutdown_reason);

	return 0;
}
EXPORT_SYMBOL(life_cycle_set_thermal_shutdown_reason);

/*
 * life_cycle_set_special_mode
 * Description: this function will set the special mode which causing device life cycle change
 */
int life_cycle_set_special_mode(life_cycle_special_mode life_cycle_special_mode)
{
	if (!p_dev_sol) {
		printk(KERN_ERR "%s: the life cycle driver is not initialized\n", __func__);
		return -1;
	}

	if (!p_dev_sol->sol_ops.write_special_mode) {
		printk(KERN_ERR "%s: no platform supported\n", __func__);
		return -1;
	} else
		p_dev_sol->sol_ops.write_special_mode(life_cycle_special_mode);

	return 0;
}
EXPORT_SYMBOL(life_cycle_set_special_mode);


extern int life_cycle_platform_init(sign_of_life_ops *sol_ops);
int __weak life_cycle_platform_init(sign_of_life_ops *sol_ops)
{
	printk(KERN_ERR "%s: no supported platform is configured\n", __func__);
	return 0;
}

static int __init dev_sol_init(void)
{
	printk(KERN_INFO "Amazon: sign of life device driver init\n");
	p_dev_sol = kzalloc(sizeof(struct dev_sol), GFP_KERNEL);
	if (!p_dev_sol) {
		printk (KERN_ERR "%s: kmalloc allocation failed\n", __func__);
		return -ENOMEM;
	}
	mutex_init(&p_dev_sol->lock);

	/* set the default life cycle reason */
	p_dev_sol->life_cycle_reason_idx = LIFE_CYCLES_DEFAULT_REASON;

	/* create the proc entry for life cycle reason */
	life_cycle_metrics_proc_init();

	/* initialize the platform dependent life cycle operation implementation */
	life_cycle_platform_init(&p_dev_sol->sol_ops);

	/* look up the life cycle reason */
	life_cycle_reason_lookup();
	printk(KERN_INFO "%s: life cycle reason is %s\n", __func__, life_cycle_reasons[p_dev_sol->life_cycle_reason_idx]);

	/* clean up the life cycle reason settings */
	if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT || get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT) {
		printk(KERN_INFO "%s: in charger mode. Don't reset LCR registers.\n", __func__);
	} else {
		printk(KERN_INFO "%s: not in charger mode. Reset LCR registers.\n", __func__);
		p_dev_sol->sol_ops.lcr_reset();
	}

	return 0;
}

static void __exit dev_sol_cleanup(void)
{
	life_cycle_metrics_proc_done();
	if (p_dev_sol)
		kfree(p_dev_sol);
}

late_initcall(dev_sol_init);
module_exit(dev_sol_cleanup);
MODULE_LICENSE("GPL v2");
