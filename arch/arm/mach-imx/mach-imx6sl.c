/*
 * Copyright (C) 2013-2014 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/opp.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/mfd/syscon/imx6q-iomuxc-gpr.h>
#ifdef CONFIG_IOHW_RECORD
#include <linux/iohw_log.h>
#endif
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_misc.h>
#include <linux/memblock.h>
#include <asm/setup.h>
#include <linux/gpio.h>
#include "common.h"
#include "cpuidle.h"
#include "hardware.h"
#include "regs-anadig.h"

/* abc123 pins related to BRCM 4343W Wifi module */
/* TODO remove these since these should be fetched from DTS JEIGHT-114 */
#define		MX6SL_abc123_WL_REG_ON	IMX_GPIO_NR(2, 15)	/* LCD_CLK - WIFI_RESET_B */
#if 0
// TODO: Fix it!
// (3,31) is the correct pin, but when set up, it causes the driver to explode during suspend for some reason.
// (maybe trying to wake up after power or bus has been pulled out from under it or something)
#define		MX6SL_abc123_WIFI_WAKE_ON_LAN_B	IMX_GPIO_NR(3, 31)	/* KEY_ROW3 */
#else
#define		MX6SL_abc123_WIFI_WAKE_ON_LAN_B	IMX_GPIO_NR(2, 17)	/* LCD_HSYNC */
#endif
//#define	MX6SL_abc123_WL_GPIO_1	IMX_GPIO_NR(5,  8)	/* SD1_DAT1 - WIFI_GPIO_1 */

#define MX6SL_abc123_WL_BOOST_ENAB  IMX_GPIO_NR(4, 23) /* Enable AMP BOOST */
#define MX6SL_abc123_WL_AMP_ENAB    IMX_GPIO_NR(3, 10) /* Enable AMP */


/*EANAB TOUCH pins*/
#define MX6SL_PIN_TOUCH_SWDL    IMX_GPIO_NR(3, 24)       /* KEY_COL0 - SWDIO touch host side 
                                                          * program pin, used on Zforce2 */
#define MX6SL_PIN_TOUCH_INTB    IMX_GPIO_NR(4, 3)        /* KEY_ROW5 - touch int */
#define MX6SL_PIN_TOUCH_RST     IMX_GPIO_NR(4, 5)        /* KEY_ROW6 - touch reset */
#define MX6SL_PIN_TOUCH_UART_TX IMX_GPIO_NR(5, 9)        /* SD1_DAT5 - BSL_RX on Zforce2 */
#define MX6SL_PIN_TOUCH_UART_RX IMX_GPIO_NR(5, 12)       /* SD1_DAT4 - BSL_TX on ZForce2 */

/*EANAB EPD PMIC enable pins*/
#define MX6SL_PIN_PM_EPD_EN 	IMX_GPIO_NR(3, 25) 	/*KEY_ROW0 - PM_EPD_EN*/
#define MX6SL_PIN_PM_EPD_ENOP 	IMX_GPIO_NR(3, 27) 	/*KEY_ROW1 - PM_EPD_ENOP*/


#define IOMUXC_SW_MUX_CTL_PAD_SD1_DATA4_OFFSET	0x244
#define IOMUXC_SW_MUX_CTL_PAD_SD1_DATA5_OFFSET  0x248
#define IOMUXC_SW_PAD_CTL_PAD_SD1_DATA4_OFFSET  0x54c
#define IOMUXC_SW_PAD_CTL_PAD_SD1_DATA5_OFFSET  0x550

#define CONFIG_GPIO 0x5
#define CONFIG_UART 0x4

static struct platform_device imx6sl_cpufreq_pdev = {
	.name = "imx6-cpufreq",
};

/* ALL WIFI changes in this file are temporary hacks and must be removed
 * see DEE-DEE-17573
 */

/* Enables Amplifier and Boost
 * TODO: DEE-17602 Enable this in imx-max98090.c
 */
static amp_boost_enable(int enable)
{
	int ret;
	pr_info("%s:iMX:MAX98090:BOARD\n", __func__);

	ret = gpio_request(MX6SL_abc123_WL_BOOST_ENAB, "BOOST_ENABLE");
	if (ret < 0) {
		pr_err("%s:max98090-amp failed to get BOOST_ENABLE %d\n",
			__func__, ret);
		return ret;
	}
	ret = gpio_request(MX6SL_abc123_WL_AMP_ENAB, "AMP_ENABLE");
	if (ret < 0) {
		pr_err("%s:iMX:MAX98090 failed to get AMP_ENABLE %d\n",
			__func__, ret);
		return ret;
	}

	ret = gpio_direction_output(MX6SL_abc123_WL_BOOST_ENAB, 0);
	if (ret < 0)  {
		pr_err("%s:iMX:MAX98090 failed to output BOOST_ENABLE %d\n",
			__func__, ret);
		return ret;
	}
	ret = gpio_direction_output(MX6SL_abc123_WL_AMP_ENAB, 0);
	if (ret < 0) {
		pr_err("%s:iMX:MAX98090 failed to output AMP_ENABLE %d\n",
			__func__, ret);
		return ret;
	}

	gpio_set_value(MX6SL_abc123_WL_BOOST_ENAB, enable);
	gpio_set_value(MX6SL_abc123_WL_AMP_ENAB, enable);

	return ret;
}

static void __init imx6sl_fec_clk_init(void)
{
	struct regmap *gpr;

	/* set FEC clock from internal PLL clock source */
	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6sl-iomuxc-gpr");
	if (!IS_ERR(gpr)) {
		regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6SL_GPR1_FEC_CLOCK_MUX2_SEL_MASK, 0);
		regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6SL_GPR1_FEC_CLOCK_MUX1_SEL_MASK, 0);
	} else
		pr_err("failed to find fsl,imx6sl-iomux-gpr regmap\n");
}

//Fred USB bringup -- need to override VBUS detection based on the USB status from PMIC
void usbotg_force_bsession(bool connected)
{
        u32 contents;
        contents = readl_relaxed(MX6Q_IO_ADDRESS(MX6Q_ANATOP_BASE_ADDR) + HW_ANADIG_USB1_VBUS_DETECT);

        contents |= BM_ANADIG_USB1_VBUS_DETECT_VBUS_OVERRIDE_EN;

        if (connected) {
                contents |= BM_ANADIG_USB1_VBUS_DETECT_BVALID_OVERRIDE;
                contents |= BM_ANADIG_USB1_VBUS_DETECT_VBUSVALID_OVERRIDE;
                contents &= ~BM_ANADIG_USB1_VBUS_DETECT_SESSEND_OVERRIDE;
        } else {
                contents &= ~BM_ANADIG_USB1_VBUS_DETECT_BVALID_OVERRIDE;
                contents &= ~BM_ANADIG_USB1_VBUS_DETECT_VBUSVALID_OVERRIDE;
                contents |= BM_ANADIG_USB1_VBUS_DETECT_SESSEND_OVERRIDE;
        }
        writel_relaxed(contents, MX6Q_IO_ADDRESS(MX6Q_ANATOP_BASE_ADDR) + HW_ANADIG_USB1_VBUS_DETECT);
}
EXPORT_SYMBOL(usbotg_force_bsession);

void touch_suspend(void) {
	/* config as gpio */
	writel_relaxed(CONFIG_GPIO, MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_MUX_CTL_PAD_SD1_DATA4_OFFSET);	
	writel_relaxed(CONFIG_GPIO, MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_MUX_CTL_PAD_SD1_DATA5_OFFSET);
	/* pad ctrl setting from hw team */
	writel_relaxed(0x4130b0,  MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_PAD_CTL_PAD_SD1_DATA4_OFFSET);
	writel_relaxed(0x4130b0,  MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_PAD_CTL_PAD_SD1_DATA5_OFFSET);
}
EXPORT_SYMBOL(touch_suspend);

void touch_resume(void) {
	/* config back to uart */
	writel_relaxed(CONFIG_UART, MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_MUX_CTL_PAD_SD1_DATA4_OFFSET);
	writel_relaxed(CONFIG_UART, MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_MUX_CTL_PAD_SD1_DATA5_OFFSET);
	/* pad ctrl setting from hw team */
	writel_relaxed(0x1b0b1,  MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_PAD_CTL_PAD_SD1_DATA4_OFFSET);
	writel_relaxed(0x1b0b1,  MX6Q_IO_ADDRESS(MX6Q_IOMUXC_BASE_ADDR) + IOMUXC_SW_PAD_CTL_PAD_SD1_DATA5_OFFSET);
}
EXPORT_SYMBOL(touch_resume);

static inline void imx6sl_fec_init(void)
{
	imx6sl_fec_clk_init();
	imx6_enet_mac_init("fsl,imx6sl-fec");
}

#if defined(CONFIG_LAB126)
int __init lab126_idme_vars_init(void);
#endif

static void __init imx6sl_init_machine(void)
{
	struct device *parent;

#if defined(CONFIG_LAB126)
	lab126_idme_vars_init();	
#endif

	mxc_arch_reset_init_dt();

	parent = imx_soc_device_init();
	if (parent == NULL)
		pr_warn("failed to initialize soc device\n");

	of_platform_populate(NULL, of_default_bus_match_table, NULL, parent);

	imx6sl_fec_init();
	imx_anatop_init();
	imx6_pm_init();

	/*FIXME remove this for EANAB JEIGHT-114 */
	amp_boost_enable(1);
}

static void __init imx6sl_opp_init(struct device *cpu_dev)
{
	struct device_node *np;
	printk(KERN_DEBUG "imx6sl_opp_init");
	np = of_find_node_by_path("/cpus/cpu@0");
	if (!np) {
		pr_warn("failed to find cpu0 node\n");
		return;
	}

	cpu_dev->of_node = np;
	if (of_init_opp_table(cpu_dev)) {
		pr_warn("failed to init OPP table\n");
		goto put_node;
	}

put_node:
	of_node_put(np);
}




static void __init imx6sl_init_late(void)
{
	struct regmap *gpr;

	/*
	 * Need to force IOMUXC irq pending to meet CCM low power mode
	 * restriction, this is recommended by hardware team.
	 */
	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6sl-iomuxc-gpr");
	if (!IS_ERR(gpr))
		regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6Q_GPR1_GINT_MASK,
			IMX6Q_GPR1_GINT_ASSERT);
	else
		pr_err("failed to find fsl,imx6sl-iomux-gpr regmap\n");

	/* Init CPUIDLE */
	imx6sl_cpuidle_init();

	if (IS_ENABLED(CONFIG_ARM_IMX6_CPUFREQ)) {
		imx6sl_opp_init(&imx6sl_cpufreq_pdev.dev);
		platform_device_register(&imx6sl_cpufreq_pdev);
	}
}

static void __init imx6sl_disable_l2_ocram(void) {
	void __iomem *iomuxc_base;
	struct device_node *np;
	unsigned int val;

	/* 
	 * If the L2 Cache as OCRAM feature is left enabled the device can
	 * crash soon after the PL310 is enabled, so this feature must be
	 * disabled early. This can't use the syscon regmap for iomux, because
	 * it doesn't exist yet.
	 */

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6sl-iomuxc-gpr");
	if (!np) {
		pr_err("failed to find fsl,imx6sl-iomuxc-gpr node.\n");
		return;
	}

	iomuxc_base = of_iomap(np, 0);
	if (!iomuxc_base) {
		of_node_put(np);
		pr_err("failed to map ifsl,imx6sl-iomuxc-gpr.\n");
		return;
	}

	val = readl_relaxed(iomuxc_base + IOMUXC_GPR11);

	if (val & IMX6SL_GPR11_OCRAM_L2_EN_MASK) {
		if (val & IMX6SL_GPR11_LOCK_OCRAM_L2_EN_MASK) {
			val &= ~IMX6SL_GPR11_LOCK_OCRAM_L2_EN_MASK;
			writel_relaxed(val, iomuxc_base + IOMUXC_GPR11);
		}

		val &= ~IMX6SL_GPR11_OCRAM_L2_EN_MASK;
		writel_relaxed(val, iomuxc_base + IOMUXC_GPR11);
	}

	if (! (val & IMX6SL_GPR11_LOCK_OCRAM_L2_EN_MASK)) {
		val |= IMX6SL_GPR11_LOCK_OCRAM_L2_EN_MASK;
		writel_relaxed(val, iomuxc_base + IOMUXC_GPR11);
	}

	iounmap(iomuxc_base);
	of_node_put(np);
}

static void __init imx6sl_map_io(void)
{
	debug_ll_io_init();
	imx6_pm_map_io();
	imx6_busfreq_map_io();
}

static void __init imx6sl_init_irq(void)
{
	imx_init_revision_from_anatop();
	imx6sl_disable_l2_ocram();
	imx_init_l2cache();
	imx_src_init();
	imx_gpc_init();
	irqchip_init();
}

static void __init imx6sl_timer_init(void)
{
	of_clk_init(NULL);
}

static const char *imx6sl_dt_compat[] __initdata = {
	"fsl,imx6sl",
	NULL,
};


/**************touch**************************/
int gpio_cyttsp_init_pins(void)
{
	int ret = 0;
	//touch pins
	ret = gpio_request(MX6SL_PIN_TOUCH_INTB, "touch_intb");
	if(unlikely(ret)) return ret;

	ret = gpio_request(MX6SL_PIN_TOUCH_RST, "touch_rst");
	if(unlikely(ret)) goto free_intb;

	gpio_direction_input(MX6SL_PIN_TOUCH_INTB);
	gpio_direction_output(MX6SL_PIN_TOUCH_RST, 1);

	return ret;
free_intb:
	gpio_free(MX6SL_PIN_TOUCH_INTB);
	return ret;

}
EXPORT_SYMBOL(gpio_cyttsp_init_pins);

/* zforce2 GPIOs setup */
int gpio_zforce_init_pins(void)
{
	int ret = 0;

	ret = gpio_request(MX6SL_PIN_TOUCH_INTB, "touch_intb");
	if(unlikely(ret)) return ret;

	ret = gpio_request(MX6SL_PIN_TOUCH_RST, "touch_rst");
	if(unlikely(ret)) goto free_intb;

	/* touch BSL programming pins */
	ret = gpio_request(MX6SL_PIN_TOUCH_SWDL, "touch_swdl");
	if(unlikely(ret)) goto free_rst;

	ret = gpio_request(MX6SL_PIN_TOUCH_UART_TX, "touch_uarttx");
	if(unlikely(ret)) goto free_swdl;

	ret = gpio_request(MX6SL_PIN_TOUCH_UART_RX, "touch_uartrx");
	if(unlikely(ret)) goto free_uarttx;

	/* GPIO Interrupt - is set as input once and for all */
	gpio_direction_input(MX6SL_PIN_TOUCH_INTB);

	/* trigger reset - active low */
	gpio_direction_output(MX6SL_PIN_TOUCH_RST, 0);

	return ret;

free_uarttx:
	gpio_free(MX6SL_PIN_TOUCH_UART_TX);
free_swdl:
	gpio_free(MX6SL_PIN_TOUCH_SWDL);
free_rst:
	gpio_free(MX6SL_PIN_TOUCH_RST);
free_intb:
	gpio_free(MX6SL_PIN_TOUCH_INTB);
	return ret;
}
EXPORT_SYMBOL(gpio_zforce_init_pins);

int gpio_zforce_reset_ena(int enable)
{
	if(enable)
		gpio_direction_output(MX6SL_PIN_TOUCH_RST, 0);
	else
		gpio_direction_input(MX6SL_PIN_TOUCH_RST);
	return 0;
}
EXPORT_SYMBOL(gpio_zforce_reset_ena);

void gpio_zforce_set_reset(int val)
{
	gpio_set_value(MX6SL_PIN_TOUCH_RST, val);
}
EXPORT_SYMBOL(gpio_zforce_set_reset);

void gpio_zforce_set_bsl_test(int val)
{
	gpio_set_value(MX6SL_PIN_TOUCH_SWDL, val);
}
EXPORT_SYMBOL(gpio_zforce_set_bsl_test);

void gpio_zforce_bslpins_ena(int enable)
{
	if(enable) {
		gpio_direction_output(MX6SL_PIN_TOUCH_SWDL, 0);
		gpio_direction_input(MX6SL_PIN_TOUCH_UART_RX);
		gpio_direction_output(MX6SL_PIN_TOUCH_UART_TX, 0);
	} else {
		gpio_direction_input(MX6SL_PIN_TOUCH_SWDL);
		gpio_direction_input(MX6SL_PIN_TOUCH_UART_TX);
		gpio_direction_input(MX6SL_PIN_TOUCH_UART_RX);
	}
}
EXPORT_SYMBOL(gpio_zforce_bslpins_ena);

void gpio_zforce_free_pins(void)
{
	gpio_free(MX6SL_PIN_TOUCH_INTB);
	gpio_free(MX6SL_PIN_TOUCH_RST);
	gpio_free(MX6SL_PIN_TOUCH_SWDL);
	gpio_free(MX6SL_PIN_TOUCH_UART_TX);
	gpio_free(MX6SL_PIN_TOUCH_UART_RX);
}
EXPORT_SYMBOL(gpio_zforce_free_pins);

/** Touch Controller IRQ setup for cyttsp and zforce2 **/
void gpio_touchcntrl_request_irq(int enable)
{
	if (enable)
		gpio_direction_input(MX6SL_PIN_TOUCH_INTB);
}
EXPORT_SYMBOL(gpio_touchcntrl_request_irq);

int gpio_touchcntrl_irq(void)
{
	return gpio_to_irq(MX6SL_PIN_TOUCH_INTB);
}
EXPORT_SYMBOL(gpio_touchcntrl_irq);

int gpio_touchcntrl_irq_get_value(void)
{
	return gpio_get_value( MX6SL_PIN_TOUCH_INTB);
}
EXPORT_SYMBOL(gpio_touchcntrl_irq_get_value);
/****/

int gpio_epd_init_pins(void)
{
	int ret = 0;

	ret = gpio_request(MX6SL_PIN_PM_EPD_EN, "epd_pm_en");
	if(unlikely(ret))
	{
		printk(KERN_ERR "Fred: Failed to request PM_EPD_EN\n");
		return ret;
	}
	ret = gpio_request(MX6SL_PIN_PM_EPD_ENOP, "epd_pm_enop");
	if(unlikely(ret))
	{
		printk(KERN_ERR "Fred: Failed to request PM_EPD_ENOP\n");
		goto free_epd_en;
	}


	/* set PM_EPD_EN and PM_EPD_ENOP to 0 */
	gpio_direction_output(MX6SL_PIN_PM_EPD_EN, 0);
	gpio_direction_output(MX6SL_PIN_PM_EPD_ENOP, 0);

	return ret;

free_epd_en:
	gpio_free(MX6SL_PIN_PM_EPD_EN);
	return ret;
}
EXPORT_SYMBOL(gpio_epd_init_pins);

void gpio_epd_enable_hv(int enable)
{
	gpio_direction_output(MX6SL_PIN_PM_EPD_EN, enable);
}
EXPORT_SYMBOL(gpio_epd_enable_hv);

void gpio_epd_enable_vcom(int enable)
{
	gpio_direction_output(MX6SL_PIN_PM_EPD_ENOP, enable);
}
EXPORT_SYMBOL(gpio_epd_enable_vcom);

void gpio_epd_free_pins(void)
{
	gpio_free(MX6SL_PIN_PM_EPD_EN);
	gpio_free(MX6SL_PIN_PM_EPD_ENOP);
}
EXPORT_SYMBOL(gpio_epd_free_pins);

extern unsigned long int ramoops_phys_addr;
extern unsigned long int ramoops_mem_size;
static void imx6sl_reserve(void)
{
	phys_addr_t phys;
	phys_addr_t max_phys;
	struct meminfo *mi;
	struct membank *bank;
#ifdef CONFIG_PSTORE_RAM
	mi = &meminfo;
	if (!mi) {
		pr_err("no memory reserve for ramoops.\n");
		return;
	}

	/* use memmory last bank for ram console store */
	bank = &mi->bank[mi->nr_banks - 1];
	if (!bank) {
		pr_err("no memory reserve for ramoops.\n");
		return;
	}
	max_phys = bank->start + bank->size;
	/* reserve 64M for uboot avoid ram console data is cleaned by uboot */
	phys = memblock_alloc_base(SZ_1M, SZ_4K, max_phys - SZ_64M);
	if (phys) {
		memblock_remove(phys, SZ_1M);
		memblock_reserve(phys, SZ_1M);
		ramoops_phys_addr = phys;
		ramoops_mem_size = SZ_1M;
	} else {
		ramoops_phys_addr = 0;
		ramoops_mem_size = 0;
		pr_err("no memory reserve for ramoops.\n");
	}
#endif

#ifdef CONFIG_IOHW_RECORD
	iohwrec_reserve_buf();
#endif
	return;
}

DT_MACHINE_START(IMX6SL, "Freescale i.MX6 SoloLite (Device Tree)")
	.map_io		= imx6sl_map_io,
	.init_irq	= imx6sl_init_irq,
	.init_time	= imx6sl_timer_init,
	.init_machine	= imx6sl_init_machine,
	.init_late      = imx6sl_init_late,
	.dt_compat	= imx6sl_dt_compat,
	.reserve     = imx6sl_reserve,
	.restart	= mxc_restart,
MACHINE_END
