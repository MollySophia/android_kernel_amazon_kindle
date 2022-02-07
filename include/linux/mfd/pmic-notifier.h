/*
 * Copyright 2004-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2016 Amazon Technologies. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __PMIC_NOTIFIER_H__
#define __PMIC_NOTIFIER_H__

#include <linux/pmic_status.h>
#include <linux/mfd/bd7181x.h>
#include <linux/notifier.h>

/*!
 * @file pmic_notifier.h
 * @brief This file contains prototypes of all the functions to be
 * defined for each PMIC chip. The implementation of these may differ
 * from PMIC chip to PMIC chip.
 *
 * @ingroup PMIC_CORE
 */

static struct atomic_notifier_head pmic_power_button_notifier_chain;
static struct atomic_notifier_head pmic_battery_notifier_chain;
static struct atomic_notifier_head pmic_charging_notifier_chain;
static struct atomic_notifier_head pmic_temp_notifier_chain;
static struct atomic_notifier_head pmic_fg_notifier_chain;
static struct atomic_notifier_head pmic_batmon_notifier_chain;

/* power */
int register_pmic_power_button_notifier(struct notifier_block *nb);
int unregister_pmic_power_button_notifier(struct notifier_block *nb);
int pmic_power_button_notifier_call_chain(unsigned long val, void *v);

/* battery */
int register_pmic_battery_notifier(struct notifier_block *nb);
int unregister_pmic_battery_notifier(struct notifier_block *nb);
int pmic_battery_notifier_call_chain(unsigned long val, void *v);

/* charging */
int register_pmic_charging_notifier(struct notifier_block *nb);
int unregister_pmic_charging_notifier(struct notifier_block *nb);
int pmic_charging_notifier_call_chain(unsigned long val, void *v);

/* temp */
int register_pmic_temp_notifier(struct notifier_block *nb);
int unregister_pmic_temp_notifier(struct notifier_block *nb);
int pmic_temp_notifier_call_chain(unsigned long val, void *v);

/* fuel gauge */
int register_pmic_fg_notifier(struct notifier_block *nb);
int unregister_pmic_fg_notifier(struct notifier_block *nb);
int pmic_fg_notifier_call_chain(unsigned long val, void *v);

/* battery monitor */
int register_pmic_batmon_notifier(struct notifier_block *nb);
int unregister_pmic_batmon_notifier(struct notifier_block *nb);
int pmic_batmon_notifier_call_chain(unsigned long val, void *v);

#endif	/* __PMIC_NOTIFIER_H__ */
