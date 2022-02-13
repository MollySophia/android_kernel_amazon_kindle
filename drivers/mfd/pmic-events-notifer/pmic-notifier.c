/*
 * Copyright 2016 Amazon Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/mfd/pmic-notifier.h>
#include <linux/pmic_status.h>
#include <linux/notifier.h>

static struct atomic_notifier_head pmic_power_button_notifier_chain;
static struct atomic_notifier_head pmic_battery_notifier_chain;
static struct atomic_notifier_head pmic_charging_notifier_chain;
static struct atomic_notifier_head pmic_temp_notifier_chain;
static struct atomic_notifier_head pmic_fg_notifier_chain;
static struct atomic_notifier_head pmic_batmon_notifier_chain;

/* PMIC Power button notifier chain */
int register_pmic_power_button_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&pmic_power_button_notifier_chain, nb);
}

int unregister_pmic_power_button_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&pmic_power_button_notifier_chain, nb);
}

int pmic_power_button_notifier_call_chain(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&pmic_power_button_notifier_chain, val, v);
}

/* PMIC battery notifier chain */
int register_pmic_battery_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&pmic_battery_notifier_chain, nb);
}

int unregister_pmic_battery_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&pmic_battery_notifier_chain, nb);
}

int pmic_battery_notifier_call_chain(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&pmic_battery_notifier_chain, val, v);
}

/* PMIC charging notifier chain */
int register_pmic_charging_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&pmic_charging_notifier_chain, nb);
}

int unregister_pmic_charging_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&pmic_charging_notifier_chain, nb);
}

int pmic_charging_notifier_call_chain(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&pmic_charging_notifier_chain, val, v);
}

/* PMIC temprature notifier chain */
int register_pmic_temp_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&pmic_temp_notifier_chain, nb);
}

int unregister_pmic_temp_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&pmic_temp_notifier_chain, nb);
}

int pmic_temp_notifier_call_chain(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&pmic_temp_notifier_chain, val, v);
}

/* PMIC fuel gauge notifier chain */
int register_pmic_fg_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&pmic_fg_notifier_chain, nb);
}

int unregister_pmic_fg_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&pmic_fg_notifier_chain, nb);
}

int pmic_fg_notifier_call_chain(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&pmic_fg_notifier_chain, val, v);
}

/* PMIC battery monitor notifier chain */
int register_pmic_batmon_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&pmic_batmon_notifier_chain, nb);
}

int unregister_pmic_batmon_notifier (struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&pmic_batmon_notifier_chain, nb);
}

int pmic_batmon_notifier_call_chain(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&pmic_batmon_notifier_chain, val, v);
}

static int __init pmic_notifier_chains_init(void)
{
	ATOMIC_INIT_NOTIFIER_HEAD(&pmic_power_button_notifier_chain);
	ATOMIC_INIT_NOTIFIER_HEAD(&pmic_battery_notifier_chain);
	ATOMIC_INIT_NOTIFIER_HEAD(&pmic_charging_notifier_chain);
	ATOMIC_INIT_NOTIFIER_HEAD(&pmic_temp_notifier_chain);
	ATOMIC_INIT_NOTIFIER_HEAD(&pmic_fg_notifier_chain);
	ATOMIC_INIT_NOTIFIER_HEAD(&pmic_batmon_notifier_chain);

	return 0;
}

subsys_initcall(pmic_notifier_chains_init);
