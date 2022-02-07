/*
 * Copyright (C) 1999 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 * Copyright (C) 2006-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Juergen Beisert, kernel@pengutronix.de
 * Copyright 2009 Ilya Yanok, Emcraft Systems Ltd, yanok@emcraft.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/system_misc.h>
#include <asm/proc-fns.h>
#include <asm/mach-types.h>
#include <asm/hardware/cache-l2x0.h>

#include "common.h"
#include "hardware.h"
#include "mx6.h"

static void __iomem *wdog_base;
static struct clk *wdog_clk;
static u32 wdog_source = 1; /* use WDOG1 default */

/* This function will set a bit on SNVS_LPGPR[3-0] bits to enter
 * special boot mode.  These bits will not clear by watchdog reset, so
 * it can be checked by bootloader to choose enter different mode.*/

#define ANDROID_BOOT_MASK		(0xF)
#define ANDROID_BOOT_UNKNOWN		(0)
#define ANDROID_BOOT_FASTBOOT		(5)
#define ANDROID_BOOT_RECOVERY		(6)
#define ANDROID_BOOT_FACTORYRESET	(7)
#define ANDROID_BOOT_KILLPILL		(0xE)
#define ANDROID_BOOT_REGULAR_REBOOT	(0xD)

#define AIPS1_ARB_BASE_ADDR		0x02000000
#define ATZ1_BASE_ADDR			AIPS1_ARB_BASE_ADDR
#define AIPS1_OFF_BASE_ADDR		(ATZ1_BASE_ADDR + 0x80000)
#define MX6_SNVS_BASE_ADDR		(AIPS1_OFF_BASE_ADDR + 0x4C000)
#define SNVS_LPGPR			0x68
#define SNVS_SIZE			(1024*16)

#define MX6_SRC_BASE_ADDR              0x020d8000
#define MX6_SRC_SIZE                   0x4000
#define SRS_OFFSET                     8

#define IMX2_WDT_WSR            0x02            /* Service Register */
#define IMX2_WDT_SEQ1           0x5555          /* -> service sequence 1 */
#define IMX2_WDT_SEQ2           0xAAAA          /* -> service sequence 2 */

static void imx2_wdt_ping(void)
{
        __raw_writew(IMX2_WDT_SEQ1, wdog_base + IMX2_WDT_WSR);
        __raw_writew(IMX2_WDT_SEQ2, wdog_base + IMX2_WDT_WSR);
}

void set_boot_mode(u32 val)
{
	u32 reg;
	void __iomem *addr;
	
	pr_err("%s: mode: %x\n", __func__,val);

	addr = ioremap(MX6_SNVS_BASE_ADDR, SNVS_SIZE);
	if (!addr) {
		pr_warn("SNVS ioremap failed!\n");
		return;
	}

	pr_err("to write to register\n");
	reg = __raw_readl(addr + SNVS_LPGPR);
	reg &= ~ANDROID_BOOT_MASK;
	reg |= (val & ANDROID_BOOT_MASK);
	__raw_writel(reg, (addr + SNVS_LPGPR));
	__iowmb();
	pr_err("read out: %d\n", __raw_readl(addr + SNVS_LPGPR));
	__iormb();

	iounmap(addr);
}

int __init get_aboot_mode(void)
{
	u32 reg;
	void __iomem *addr;

	addr = ioremap(MX6_SNVS_BASE_ADDR, SNVS_SIZE);
	if (!addr) {
		pr_warn("SNVS ioremap failed!\n");
		return -ENOMEM;
	}

	reg = __raw_readl(addr + SNVS_LPGPR);
	reg &= ANDROID_BOOT_MASK;
	__iormb();
	iounmap(addr);

	return reg;
}

int __init get_imx_reset_reason(void)
{
	u32 reg;
	void __iomem *addr;

	addr = ioremap(MX6_SRC_BASE_ADDR, MX6_SRC_SIZE);
	if (!addr) {
		pr_warn("SRC ioremap failed!\n");
		return -ENOMEM;
	}

	reg = __raw_readl(addr + SRS_OFFSET);
	__iormb();

	/*
	 * Clear SRSR after reading.
	 */
	__raw_writel(reg, addr + SRS_OFFSET);
	__iowmb();

	iounmap(addr);

	return reg;
}

static void arch_reset_special_mode(char mode, const char *cmd)
{
	pr_err("%s: cmd: %s\n", __func__,cmd);

	if (cmd && strcmp(cmd, "recovery") == 0)
		set_boot_mode(ANDROID_BOOT_RECOVERY);
	else if (cmd && strcmp(cmd, "bootloader") == 0)
		set_boot_mode(ANDROID_BOOT_FASTBOOT);
	else if (cmd && strcmp(cmd, "factoryreset") == 0)
		set_boot_mode(ANDROID_BOOT_FACTORYRESET);
	else if (cmd && strcmp(cmd, "killpill") == 0)
		set_boot_mode(ANDROID_BOOT_KILLPILL);
	else
		set_boot_mode(ANDROID_BOOT_REGULAR_REBOOT);
}
/*
 * Reset the system. It is called by machine_restart().
 */
void mxc_restart(char mode, const char *cmd)
{
	unsigned int wcr_enable;

	arch_reset_special_mode(mode, cmd);

	if (wdog_clk)
		clk_enable(wdog_clk);

	if (cpu_is_mx1())
		wcr_enable = (1 << 0);
	/*
	 * Some i.MX6 boards use WDOG2 to reset external pmic in bypass mode,
	 * so do WDOG2 reset here. Do not set SRS, since we will
	 * trigger external POR later. Use WDOG1 to reset in ldo-enable
	 * mode. You can set it by "fsl,wdog-reset" in dts.
	 * For i.MX6SX we have to trigger wdog-reset to reset QSPI-NOR flash to
	 * workaround qspi-nor reboot issue whatever ldo-bypass or not.
	 */
	else if ((wdog_source == 2 && (cpu_is_imx6q() || cpu_is_imx6dl() ||
			cpu_is_imx6sl())) || cpu_is_imx6sx())
		wcr_enable = 0x14;
	else
		wcr_enable = (1 << 2);

	/* Assert WDOG_B on hardware timeout */
	wcr_enable |= (1 << 3);

	/* Don't software assert WDOG_B and System reset */
	wcr_enable |= (3 << 4);

	/* Assert SRS signal */
	__raw_writew(wcr_enable, wdog_base);
	/* write twice to ensure the request will not get ignored */
	__raw_writew(wcr_enable, wdog_base);

	imx2_wdt_ping();

	/* wait for reset to assert... */
	mdelay(500);

	pr_err("%s: Watchdog reset failed to assert reset\n", __func__);

	/* delay to allow the serial port to show the message */
	mdelay(50);

	/* we'll take a jump through zero as a poor second */
	soft_restart(0);
}

void __init mxc_arch_reset_init(void __iomem *base)
{
	wdog_base = base;

	wdog_clk = clk_get_sys("imx2-wdt.0", NULL);
	if (IS_ERR(wdog_clk)) {
		pr_warn("%s: failed to get wdog clock\n", __func__);
		wdog_clk = NULL;
		return;
	}

	clk_prepare(wdog_clk);
}

void __init mxc_arch_reset_init_dt(void)
{
	struct device_node *np = NULL;

	if (cpu_is_imx6q() || cpu_is_imx6dl())
		np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-gpc");
	else if (cpu_is_imx6sl())
		np = of_find_compatible_node(NULL, NULL, "fsl,imx6sl-gpc");

	if (np)
		of_property_read_u32(np, "fsl,wdog-reset", &wdog_source);
	pr_info("Use WDOG%d as reset source\n", wdog_source);

	np = of_find_compatible_node(NULL, NULL, "fsl,imx21-wdt");
	wdog_base = of_iomap(np, 0);
	WARN_ON(!wdog_base);

	/* Some i.MX6 boards use WDOG2 to reset board in ldo-bypass mode */
	if (wdog_source == 2 && (cpu_is_imx6q() || cpu_is_imx6dl() ||
		cpu_is_imx6sl())) {
		np = of_find_compatible_node(np, NULL, "fsl,imx21-wdt");
		wdog_base = of_iomap(np, 0);
		WARN_ON(!wdog_base);
	}

	wdog_clk = of_clk_get(np, 0);
	if (IS_ERR(wdog_clk)) {
		pr_warn("%s: failed to get wdog clock\n", __func__);
		wdog_clk = NULL;
		return;
	}

	clk_prepare(wdog_clk);
}

#ifdef CONFIG_CACHE_L2X0
void __init imx_init_l2cache(void)
{
	void __iomem *l2x0_base;
	struct device_node *np;
	unsigned int val;

	np = of_find_compatible_node(NULL, NULL, "arm,pl310-cache");
	if (!np)
		goto out;

	l2x0_base = of_iomap(np, 0);
	if (!l2x0_base) {
		of_node_put(np);
		goto out;
	}

	/* Configure the L2 PREFETCH and POWER registers */
	val = readl_relaxed(l2x0_base + L2X0_PREFETCH_CTRL);
	val |= 0x30000000;
	/*
	 * The L2 cache controller(PL310) version on the i.MX6D/Q is r3p1-50rel0
	 * The L2 cache controller(PL310) version on the i.MX6DL/SOLO/SL is r3p2
	 * But according to ARM PL310 errata: 752271
	 * ID: 752271: Double linefill feature can cause data corruption
	 * Fault Status: Present in: r3p0, r3p1, r3p1-50rel0. Fixed in r3p2
	 * Workaround: The only workaround to this erratum is to disable the
	 * double linefill feature. This is the default behavior.
	 */
	if (!of_machine_is_compatible("fsl,imx6q"))
		val |= 0x40800000;
	writel_relaxed(val, l2x0_base + L2X0_PREFETCH_CTRL);
	val = L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
	writel_relaxed(val, l2x0_base + L2X0_POWER_CTRL);

	iounmap(l2x0_base);
	of_node_put(np);

out:
	l2x0_of_init(0, ~0UL);
}
#endif
