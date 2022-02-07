/*
 * Copyright (C) 2013-2014 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define CCM_CCDR_OFFSET		0x4
#define ANATOP_PLL_USB1		0x10
#define ANATOP_PLL_USB2		0x20
#define ANATOP_PLL_ENET		0xE0
#define ANATOP_PLL_BYPASS_OFFSET	(1 << 16)
#define ANATOP_PLL_ENABLE_OFFSET	(1 << 13)
#define ANATOP_PLL_POWER_OFFSET	(1 << 12)
#define ANATOP_PFD_480n_OFFSET	0xf0
#define ANATOP_PFD_528n_OFFSET	0x100
#define PFD0_CLKGATE			(1 << 7)
#define PFD1_CLK_GATE			(1 << 15)
#define PFD2_CLK_GATE			(1 << 23)
#define PFD3_CLK_GATE			(1 << 31)
#define CCDR_CH0_HS_BYP		17
#define OSC_RATE			24000000

#define CCM_CCGR_OFFSET(index)  (index * 2)

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <dt-bindings/clock/imx6sl-clock.h>
#include <linux/clk-private.h>

#include "clk.h"
#include "common.h"

static bool uart_from_osc = 0;
static const char const *step_sels[]		= { "osc", "pll2_pfd2", };
static const char const *pll1_sw_sels[]		= { "pll1_sys", "step", };
static const char const *ocram_alt_sels[]	= { "pll2_pfd2", "pll3_pfd1", };
static const char const *ocram_sels[]		= { "periph", "ocram_alt_sel", };
static const char const *pre_periph_sels[]	= { "pll2_bus", "pll2_pfd2", "pll2_pfd0", "pll2_198m", };
static const char const *periph_clk2_sels[]	= { "pll3_usb_otg", "osc", "osc", "dummy", };
static const char const *periph2_clk2_sels[]	= { "pll3_usb_otg", "pll2_bus", };
static const char const *periph_sels[]		= { "pre_periph_sel", "periph_clk2", };
static const char const *periph2_sels[]		= { "pre_periph2_sel", "periph2_clk2", };
static const char const *csi_sels[]		= { "osc", "pll2_pfd2", "pll3_120m", "pll3_pfd1", };
static const char const *lcdif_axi_sels[]	= { "pll2_bus", "pll2_pfd2", "pll3_usb_otg", "pll3_pfd1", };
static const char const *usdhc_sels[]		= { "pll2_pfd2", "pll2_pfd0", };
static const char const *ssi_sels[]		= { "pll3_pfd2", "pll3_pfd3", "pll4_audio_div", "dummy", };
static const char const *perclk_sels[]		= { "ipg", "osc", };
static const char const *epdc_axi_sels[]	= { "pll2_bus", "pll3_usb_otg", "pll5_video_div", "pll2_pfd0", "pll2_pfd2", "pll3_pfd2", };
static const char const *pxp_axi_sels[]		= { "pll2_bus", "pll3_usb_otg", "pll5_video_div", "pll2_pfd0", "pll2_pfd2", "pll3_pfd3", };
static const char const *gpu2d_ovg_sels[]	= { "pll3_pfd1", "pll3_usb_otg", "pll2_bus", "pll2_pfd2", };
static const char const *gpu2d_sels[]		= { "pll2_pfd2", "pll3_usb_otg", "pll3_pfd1", "pll2_bus", };
static const char const *lcdif_pix_sels[]	= { "pll2_bus", "pll3_usb_otg", "pll5_video_div", "pll2_pfd0", "pll3_pfd0", "pll3_pfd1", };
static const char const *epdc_pix_sels[]	= { "pll2_bus", "pll3_usb_otg", "pll5_video_div", "pll2_pfd0", "pll2_pfd1", "pll3_pfd1", };
static const char const *audio_sels[]		= { "pll4_audio_div", "pll3_pfd2", "pll3_pfd3", "pll3_usb_otg", };
static const char const *ecspi_sels[]		= { "pll3_60m", "osc", };
static const char const *uart_sels[]		= { "pll3_80m", "uart_osc_4M", };

static struct clk_div_table clk_enet_ref_table[] = {
	{ .val = 0, .div = 20, },
	{ .val = 1, .div = 10, },
	{ .val = 2, .div = 5, },
	{ .val = 3, .div = 4, },
	{ }
};

static struct clk_div_table post_div_table[] = {
	{ .val = 2, .div = 1, },
	{ .val = 1, .div = 2, },
	{ .val = 0, .div = 4, },
	{ }
};

static struct clk_div_table video_div_table[] = {
	{ .val = 0, .div = 1, },
	{ .val = 1, .div = 2, },
	{ .val = 2, .div = 1, },
	{ .val = 3, .div = 4, },
	{ }
};

static struct clk *clks[IMX6SL_CLK_CLK_END];
static struct clk_onecell_data clk_data;
static u32 cur_arm_podf;
static u32 pll1_org_rate;

extern int low_bus_freq_mode;
extern int audio_bus_freq_mode;

/*
 * On MX6SL, need to ensure that the ARM:IPG clock ratio is maintained
 * within 12:5 when the clocks to ARM are gated when the SOC enters
 * WAIT mode. This is necessary to avoid WAIT mode issue (an early
 * interrupt waking up the ARM).
 * This function will set the ARM clk to max value within the 12:5 limit.
 */
void imx6sl_set_wait_clk(bool enter)
{
	u32 parent_rate;

	if (enter) {
		u32 wait_podf;
		u32 new_parent_rate = OSC_RATE;
		u32 ipg_rate = clk_get_rate(clks[IMX6SL_CLK_IPG]);
		u32 max_arm_wait_clk = (12 * ipg_rate) / 5;
		parent_rate = clk_get_rate(clks[IMX6SL_CLK_PLL1_SW]);
		cur_arm_podf = parent_rate / clk_get_rate(clks[IMX6SL_CLK_ARM]);
		if (low_bus_freq_mode) {
			/*
			 * IPG clk is at 12MHz at this point, we can only run
			 * ARM at a max of 28.8MHz. So we need to set ARM
			 * to run from the 24MHz OSC, as there is no way to
			 * get 28.8MHz when ARM is sourced from PLL1.
			 */
			imx_clk_set_parent(clks[IMX6SL_CLK_STEP],
							clks[IMX6SL_CLK_OSC]);
			imx_clk_set_parent(clks[IMX6SL_CLK_PLL1_SW],
							clks[IMX6SL_CLK_STEP]);
		} else if (audio_bus_freq_mode) {
			/*
			 * In this mode ARM is from PLL2_PFD2 (396MHz),
			 * but IPG is at 12MHz. Need to switch ARM to run
			 * from the bypassed PLL1 clocks so that we can run
			 * ARM at 24MHz.
			 */
			pll1_org_rate = clk_get_rate(clks[IMX6SL_CLK_PLL1_SYS]);
			/* Ensure PLL1 is at 24MHz. */
			imx_clk_set_rate(clks[IMX6SL_CLK_PLL1_SYS], OSC_RATE);
			imx_clk_set_parent(clks[IMX6SL_CLK_PLL1_SW], clks[IMX6SL_CLK_PLL1_SYS]);
		} else
			new_parent_rate = clk_get_rate(clks[IMX6SL_CLK_PLL1_SW]);
		wait_podf = (new_parent_rate + max_arm_wait_clk - 1) /
						max_arm_wait_clk;

		imx_clk_set_rate(clks[IMX6SL_CLK_ARM], new_parent_rate / wait_podf);
	} else {
		if (low_bus_freq_mode)
			/* Move ARM back to PLL1. */
			imx_clk_set_parent(clks[IMX6SL_CLK_PLL1_SW],
				clks[IMX6SL_CLK_PLL1_SYS]);
		else if (audio_bus_freq_mode) {
			/* Move ARM back to PLL2_PFD2 via STEP_CLK. */
			imx_clk_set_parent(clks[IMX6SL_CLK_PLL1_SW], clks[IMX6SL_CLK_STEP]);
			imx_clk_set_rate(clks[IMX6SL_CLK_PLL1_SYS], pll1_org_rate);
		}
		parent_rate = clk_get_rate(clks[IMX6SL_CLK_PLL1_SW]);
		imx_clk_set_rate(clks[IMX6SL_CLK_ARM], parent_rate / cur_arm_podf);
	}
}

void bt_uart_switch_parent(int bt_on)
{
	int i = 0;
	if(uart_from_osc) {
		while((clks[IMX6SL_CLK_UART_SERIAL])->enable_count) {
			clk_disable(clks[IMX6SL_CLK_UART_SERIAL]);
		}
		while((clks[IMX6SL_CLK_UART_SERIAL])->prepare_count) {
			clk_unprepare(clks[IMX6SL_CLK_UART_SERIAL]);
			i++;
		}

		if(bt_on)
			imx_clk_set_parent(clks[IMX6SL_CLK_UART_SEL], clks[IMX6SL_CLK_PLL3_80M] );
		else
			imx_clk_set_parent(clks[IMX6SL_CLK_UART_SEL], clks[IMX6SL_CLK_UART_OSC_4M]);

		while(i--)
			clk_prepare_enable(clks[IMX6SL_CLK_UART_SERIAL]);
	}
}
EXPORT_SYMBOL(bt_uart_switch_parent);

static int __init setup_uart_clk(char *uart_rate)
{
	uart_from_osc = true;
	return 1;
}

__setup("uart_at_4M", setup_uart_clk);

static void __init imx6sl_clocks_init(struct device_node *ccm_node)
{
	struct device_node *np;
	void __iomem *base;
	int irq;
	int i;
	u32 reg;

	clks[IMX6SL_CLK_DUMMY] = imx_clk_fixed("dummy", 0);
	clks[IMX6SL_CLK_CKIL] = imx_obtain_fixed_clock("ckil", 0);
	clks[IMX6SL_CLK_OSC] = imx_obtain_fixed_clock("osc", 0);

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6sl-anatop");
	base = of_iomap(np, 0);
	WARN_ON(!base);

	/*                                             type               name            parent  base         div_mask */
	clks[IMX6SL_CLK_PLL1_SYS]      = imx_clk_pllv3(IMX_PLLV3_SYS,	  "pll1_sys",	   "osc", base,        0x7f, true);
	clks[IMX6SL_CLK_PLL2_BUS]      = imx_clk_pllv3(IMX_PLLV3_GENERIC, "pll2_bus",	   "osc", base + 0x30, 0x1, true);
	clks[IMX6SL_CLK_PLL3_USB_OTG]  = imx_clk_pllv3(IMX_PLLV3_USB,	  "pll3_usb_otg",  "osc", base + 0x10, 0x3, false);
	clks[IMX6SL_CLK_PLL4_AUDIO]    = imx_clk_pllv3(IMX_PLLV3_AV,	  "pll4_audio",	   "osc", base + 0x70, 0x7f, false);
	clks[IMX6SL_CLK_PLL5_VIDEO]    = imx_clk_pllv3(IMX_PLLV3_AV,	  "pll5_video",	   "osc", base + 0xa0, 0x7f, false);
	clks[IMX6SL_CLK_PLL6_ENET]     = imx_clk_pllv3(IMX_PLLV3_ENET,	  "pll6_enet",	   "osc", base + 0xe0, 0x3, false);
	clks[IMX6SL_CLK_PLL7_USB_HOST] = imx_clk_pllv3(IMX_PLLV3_USB,     "pll7_usb_host", "osc", base + 0x20, 0x3, false);

	/*
	 * usbphy1 and usbphy2 are implemented as dummy gates using reserve
	 * bit 20.  They are used by phy driver to keep the refcount of
	 * parent PLL correct. usbphy1_gate and usbphy2_gate only needs to be
	 * turned on during boot, and software will not need to control it
	 * anymore after that.
	 */
	clks[IMX6SL_CLK_USBPHY1]      = imx_clk_gate("usbphy1",      "pll3_usb_otg",  base + 0x10, 20);
	clks[IMX6SL_CLK_USBPHY2]      = imx_clk_gate("usbphy2",      "pll7_usb_host", base + 0x20, 20);
	clks[IMX6SL_CLK_USBPHY1_GATE] = imx_clk_gate("usbphy1_gate", "dummy",         base + 0x10, 6);
	clks[IMX6SL_CLK_USBPHY2_GATE] = imx_clk_gate("usbphy2_gate", "dummy",         base + 0x20, 6);

	/*                                                           dev   name              parent_name      flags                reg        shift width div: flags, div_table lock */
	clks[IMX6SL_CLK_PLL4_POST_DIV]  = clk_register_divider_table(NULL, "pll4_post_div",  "pll4_audio",    CLK_SET_RATE_PARENT, base + 0x70,  19, 2,   0, post_div_table, &imx_ccm_lock);
	clks[IMX6SL_CLK_PLL4_AUDIO_DIV] =       clk_register_divider(NULL, "pll4_audio_div", "pll4_post_div", CLK_SET_RATE_PARENT, base + 0x170, 15, 1,   0, &imx_ccm_lock);
	clks[IMX6SL_CLK_PLL5_POST_DIV]  = clk_register_divider_table(NULL, "pll5_post_div",  "pll5_video",    CLK_SET_RATE_PARENT, base + 0xa0,  19, 2,   0, post_div_table, &imx_ccm_lock);
	clks[IMX6SL_CLK_PLL5_VIDEO_DIV] = clk_register_divider_table(NULL, "pll5_video_div", "pll5_post_div", CLK_SET_RATE_PARENT, base + 0x170, 30, 2,   0, video_div_table, &imx_ccm_lock);
	clks[IMX6SL_CLK_ENET_REF]       = clk_register_divider_table(NULL, "enet_ref",       "pll6_enet",     0,                   base + 0xe0,  0,  2,   0, clk_enet_ref_table, &imx_ccm_lock);

	/*                                       name         parent_name     reg           idx */
	clks[IMX6SL_CLK_PLL2_PFD0] = imx_clk_pfd("pll2_pfd0", "pll2_bus",     base + 0x100, 0);
	clks[IMX6SL_CLK_PLL2_PFD1] = imx_clk_pfd("pll2_pfd1", "pll2_bus",     base + 0x100, 1);
	clks[IMX6SL_CLK_PLL2_PFD2] = imx_clk_pfd("pll2_pfd2", "pll2_bus",     base + 0x100, 2);
	clks[IMX6SL_CLK_PLL3_PFD0] = imx_clk_pfd("pll3_pfd0", "pll3_usb_otg", base + 0xf0,  0);
	clks[IMX6SL_CLK_PLL3_PFD1] = imx_clk_pfd("pll3_pfd1", "pll3_usb_otg", base + 0xf0,  1);
	clks[IMX6SL_CLK_PLL3_PFD2] = imx_clk_pfd("pll3_pfd2", "pll3_usb_otg", base + 0xf0,  2);
	clks[IMX6SL_CLK_PLL3_PFD3] = imx_clk_pfd("pll3_pfd3", "pll3_usb_otg", base + 0xf0,  3);

	/*                                                       name         parent_name     mult div */
	clks[IMX6SL_CLK_PLL2_198M]    = imx_clk_fixed_factor("pll2_198m", "pll2_pfd2",      1, 2);
	clks[IMX6SL_CLK_PLL3_120M]    = imx_clk_fixed_factor("pll3_120m", "pll3_usb_otg",   1, 4);
	clks[IMX6SL_CLK_PLL3_80M]     = imx_clk_fixed_factor("pll3_80m",  "pll3_usb_otg",   1, 6);
	clks[IMX6SL_CLK_PLL3_60M]     = imx_clk_fixed_factor("pll3_60m",  "pll3_usb_otg",   1, 8);
	clks[IMX6SL_CLK_UART_OSC_4M]  = imx_clk_fixed_factor("uart_osc_4M", "osc",          1, 6);

	/* Ensure all PFDs but PLL2_PFD2 are disabled. */
	reg = readl_relaxed(base + ANATOP_PFD_480n_OFFSET);
	reg |= (PFD0_CLKGATE | PFD1_CLK_GATE | PFD2_CLK_GATE | PFD3_CLK_GATE);
	writel_relaxed(reg, base + ANATOP_PFD_480n_OFFSET);
	reg = readl_relaxed(base + ANATOP_PFD_528n_OFFSET);
	reg |= (PFD0_CLKGATE | PFD1_CLK_GATE);
	writel_relaxed(reg, base + ANATOP_PFD_528n_OFFSET);

	/* Ensure Unused PLLs are disabled. */
	reg = readl_relaxed(base + ANATOP_PLL_USB1);
	reg |= ANATOP_PLL_BYPASS_OFFSET;
	reg &= ~(ANATOP_PLL_ENABLE_OFFSET | ANATOP_PLL_POWER_OFFSET);
	writel_relaxed(reg, base + ANATOP_PLL_USB1);

	reg = readl_relaxed(base + ANATOP_PLL_USB2);
	reg |= ANATOP_PLL_BYPASS_OFFSET;
	reg &= ~(ANATOP_PLL_ENABLE_OFFSET | ANATOP_PLL_POWER_OFFSET);
	writel_relaxed(reg, base + ANATOP_PLL_USB2);

	reg = readl_relaxed(base + ANATOP_PLL_ENET);
	reg |= (ANATOP_PLL_BYPASS_OFFSET | ANATOP_PLL_POWER_OFFSET);
	reg &= ~ANATOP_PLL_ENABLE_OFFSET;
	writel_relaxed(reg, base + ANATOP_PLL_ENET);

	np = ccm_node;
	base = of_iomap(np, 0);
	WARN_ON(!base);
	imx6_pm_set_ccm_base(base);

	/*                                              name                reg       shift width parent_names     num_parents */
	clks[IMX6SL_CLK_STEP]             = imx_clk_mux("step",             base + 0xc,  8,  1, step_sels,         ARRAY_SIZE(step_sels));
	clks[IMX6SL_CLK_PLL1_SW]          = imx_clk_mux_glitchless("pll1_sw", base + 0xc, 2, 1, pll1_sw_sels,      ARRAY_SIZE(pll1_sw_sels));
	clks[IMX6SL_CLK_OCRAM_ALT_SEL]    = imx_clk_mux("ocram_alt_sel",    base + 0x14, 7,  1, ocram_alt_sels,    ARRAY_SIZE(ocram_alt_sels));
	clks[IMX6SL_CLK_OCRAM_SEL]        = imx_clk_mux_glitchless("ocram_sel", base + 0x14, 6, 1, ocram_sels,     ARRAY_SIZE(ocram_sels));
	clks[IMX6SL_CLK_PRE_PERIPH2_SEL]  = imx_clk_mux_bus("pre_periph2_sel",  base + 0x18, 21, 2, pre_periph_sels,   ARRAY_SIZE(pre_periph_sels));
	clks[IMX6SL_CLK_PRE_PERIPH_SEL]   = imx_clk_mux_bus("pre_periph_sel",   base + 0x18, 18, 2, pre_periph_sels,   ARRAY_SIZE(pre_periph_sels));
	clks[IMX6SL_CLK_PERIPH2_CLK2_SEL] = imx_clk_mux_bus("periph2_clk2_sel", base + 0x18, 20, 1, periph2_clk2_sels, ARRAY_SIZE(periph2_clk2_sels));
	clks[IMX6SL_CLK_PERIPH_CLK2_SEL]  = imx_clk_mux_bus("periph_clk2_sel",  base + 0x18, 12, 2, periph_clk2_sels,  ARRAY_SIZE(periph_clk2_sels));
	clks[IMX6SL_CLK_CSI_SEL]          = imx_clk_mux("csi_sel",          base + 0x3c, 9,  2, csi_sels,          ARRAY_SIZE(csi_sels));
	clks[IMX6SL_CLK_LCDIF_AXI_SEL]    = imx_clk_mux("lcdif_axi_sel",    base + 0x3c, 14, 2, lcdif_axi_sels,    ARRAY_SIZE(lcdif_axi_sels));
	clks[IMX6SL_CLK_USDHC1_SEL]       = imx_clk_fixup_mux("usdhc1_sel", base + 0x1c, 16, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels),  imx_cscmr1_fixup);
	clks[IMX6SL_CLK_USDHC2_SEL]       = imx_clk_fixup_mux("usdhc2_sel", base + 0x1c, 17, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels),  imx_cscmr1_fixup);
	clks[IMX6SL_CLK_USDHC3_SEL]       = imx_clk_fixup_mux("usdhc3_sel", base + 0x1c, 18, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels),  imx_cscmr1_fixup);
	clks[IMX6SL_CLK_USDHC4_SEL]       = imx_clk_fixup_mux("usdhc4_sel", base + 0x1c, 19, 1, usdhc_sels,        ARRAY_SIZE(usdhc_sels),  imx_cscmr1_fixup);
	clks[IMX6SL_CLK_SSI1_SEL]         = imx_clk_fixup_mux("ssi1_sel",   base + 0x1c, 10, 2, ssi_sels,          ARRAY_SIZE(ssi_sels),    imx_cscmr1_fixup);
	clks[IMX6SL_CLK_SSI2_SEL]         = imx_clk_fixup_mux("ssi2_sel",   base + 0x1c, 12, 2, ssi_sels,          ARRAY_SIZE(ssi_sels),    imx_cscmr1_fixup);
	clks[IMX6SL_CLK_SSI3_SEL]         = imx_clk_fixup_mux("ssi3_sel",   base + 0x1c, 14, 2, ssi_sels,          ARRAY_SIZE(ssi_sels),    imx_cscmr1_fixup);
	clks[IMX6SL_CLK_PERCLK_SEL]       = imx_clk_fixup_mux("perclk_sel", base + 0x1c, 6,  1, perclk_sels,       ARRAY_SIZE(perclk_sels), imx_cscmr1_fixup);
	clks[IMX6SL_CLK_PXP_AXI_SEL]      = imx_clk_mux("pxp_axi_sel",      base + 0x34, 6,  3, pxp_axi_sels,      ARRAY_SIZE(pxp_axi_sels));
	clks[IMX6SL_CLK_EPDC_AXI_SEL]     = imx_clk_mux("epdc_axi_sel",     base + 0x34, 15, 3, epdc_axi_sels,     ARRAY_SIZE(epdc_axi_sels));
	clks[IMX6SL_CLK_GPU2D_OVG_SEL]    = imx_clk_mux("gpu2d_ovg_sel",    base + 0x18, 4,  2, gpu2d_ovg_sels,    ARRAY_SIZE(gpu2d_ovg_sels));
	clks[IMX6SL_CLK_GPU2D_SEL]        = imx_clk_mux("gpu2d_sel",        base + 0x18, 8,  2, gpu2d_sels,        ARRAY_SIZE(gpu2d_sels));
	clks[IMX6SL_CLK_LCDIF_PIX_SEL]    = imx_clk_mux_flags("lcdif_pix_sel",    base + 0x38, 6,  3, lcdif_pix_sels,    ARRAY_SIZE(lcdif_pix_sels), CLK_SET_RATE_PARENT);
	clks[IMX6SL_CLK_EPDC_PIX_SEL]     = imx_clk_mux_flags("epdc_pix_sel",     base + 0x38, 15, 3, epdc_pix_sels,     ARRAY_SIZE(epdc_pix_sels), CLK_SET_RATE_PARENT);
	clks[IMX6SL_CLK_SPDIF0_SEL]       = imx_clk_mux("spdif0_sel",       base + 0x30, 20, 2, audio_sels,        ARRAY_SIZE(audio_sels));
	clks[IMX6SL_CLK_SPDIF1_SEL]       = imx_clk_mux("spdif1_sel",       base + 0x30, 7,  2, audio_sels,        ARRAY_SIZE(audio_sels));
	clks[IMX6SL_CLK_EXTERN_AUDIO_SEL] = imx_clk_mux_flags("extern_audio_sel", base + 0x20, 19, 2, audio_sels,        ARRAY_SIZE(audio_sels), CLK_SET_RATE_PARENT);
	clks[IMX6SL_CLK_ECSPI_SEL]        = imx_clk_mux("ecspi_sel",        base + 0x38, 18, 1, ecspi_sels,        ARRAY_SIZE(ecspi_sels));
	clks[IMX6SL_CLK_UART_SEL]         = imx_clk_mux("uart_sel",         base + 0x24, 6,  1, uart_sels,         ARRAY_SIZE(uart_sels));

	/*                                          name       reg        shift width busy: reg, shift parent_names  num_parents */
	clks[IMX6SL_CLK_PERIPH]  = imx_clk_busy_mux("periph",  base + 0x14, 25,  1,   base + 0x48, 5,  periph_sels,  ARRAY_SIZE(periph_sels));
	clks[IMX6SL_CLK_PERIPH2] = imx_clk_busy_mux("periph2", base + 0x14, 26,  1,   base + 0x48, 3,  periph2_sels, ARRAY_SIZE(periph2_sels));

	/*                                                   name                 parent_name          reg       shift width */
	clks[IMX6SL_CLK_OCRAM_PODF]        = imx_clk_busy_divider("ocram_podf", "ocram_sel", base + 0x14, 16, 3, base + 0x48, 0);
	clks[IMX6SL_CLK_PERIPH_CLK2]  = imx_clk_divider("periph_clk2",  "periph_clk2_sel",   base + 0x14, 27, 3);
	clks[IMX6SL_CLK_PERIPH2_CLK2] = imx_clk_divider("periph2_clk2", "periph2_clk2_sel",  base + 0x14, 0,  3);
	clks[IMX6SL_CLK_IPG]               = imx_clk_divider("ipg",               "ahb",               base + 0x14, 8,  2);
	clks[IMX6SL_CLK_CSI_PODF]          = imx_clk_divider("csi_podf",          "csi_sel",           base + 0x3c, 11, 3);
	clks[IMX6SL_CLK_LCDIF_AXI_PODF]    = imx_clk_divider("lcdif_axi_podf",    "lcdif_axi_sel",     base + 0x3c, 16, 3);
	clks[IMX6SL_CLK_USDHC1_PODF]       = imx_clk_divider("usdhc1_podf",       "usdhc1_sel",        base + 0x24, 11, 3);
	clks[IMX6SL_CLK_USDHC2_PODF]       = imx_clk_divider("usdhc2_podf",       "usdhc2_sel",        base + 0x24, 16, 3);
	clks[IMX6SL_CLK_USDHC3_PODF]       = imx_clk_divider("usdhc3_podf",       "usdhc3_sel",        base + 0x24, 19, 3);
	clks[IMX6SL_CLK_USDHC4_PODF]       = imx_clk_divider("usdhc4_podf",       "usdhc4_sel",        base + 0x24, 22, 3);
	clks[IMX6SL_CLK_SSI1_PRED]         = imx_clk_divider("ssi1_pred",         "ssi1_sel",          base + 0x28, 6,  3);
	clks[IMX6SL_CLK_SSI1_PODF]         = imx_clk_divider("ssi1_podf",         "ssi1_pred",         base + 0x28, 0,  6);
	clks[IMX6SL_CLK_SSI2_PRED]         = imx_clk_divider("ssi2_pred",         "ssi2_sel",          base + 0x2c, 6,  3);
	clks[IMX6SL_CLK_SSI2_PODF]         = imx_clk_divider("ssi2_podf",         "ssi2_pred",         base + 0x2c, 0,  6);
	clks[IMX6SL_CLK_SSI3_PRED]         = imx_clk_divider("ssi3_pred",         "ssi3_sel",          base + 0x28, 22, 3);
	clks[IMX6SL_CLK_SSI3_PODF]         = imx_clk_divider("ssi3_podf",         "ssi3_pred",         base + 0x28, 16, 6);
	clks[IMX6SL_CLK_PERCLK]            = imx_clk_fixup_divider("perclk",      "perclk_sel",        base + 0x1c, 0,  6, imx_cscmr1_fixup);
	clks[IMX6SL_CLK_PXP_AXI_PODF]      = imx_clk_divider("pxp_axi_podf",      "pxp_axi_sel",       base + 0x34, 3,  3);
	clks[IMX6SL_CLK_EPDC_AXI_PODF]     = imx_clk_divider("epdc_axi_podf",     "epdc_axi_sel",      base + 0x34, 12, 3);
	clks[IMX6SL_CLK_GPU2D_OVG_PODF]    = imx_clk_divider("gpu2d_ovg_podf",    "gpu2d_ovg_sel",     base + 0x18, 26, 3);
	clks[IMX6SL_CLK_GPU2D_PODF]        = imx_clk_divider("gpu2d_podf",        "gpu2d_sel",         base + 0x18, 29, 3);
	clks[IMX6SL_CLK_LCDIF_PIX_PRED]    = imx_clk_divider("lcdif_pix_pred",    "lcdif_pix_sel",     base + 0x38, 3,  3);
	clks[IMX6SL_CLK_EPDC_PIX_PRED]     = imx_clk_divider("epdc_pix_pred",     "epdc_pix_sel",      base + 0x38, 12, 3);
	clks[IMX6SL_CLK_LCDIF_PIX_PODF]    = imx_clk_fixup_divider("lcdif_pix_podf", "lcdif_pix_pred", base + 0x1c, 20, 3, imx_cscmr1_fixup);
	clks[IMX6SL_CLK_EPDC_PIX_PODF]     = imx_clk_divider("epdc_pix_podf",     "epdc_pix_pred",     base + 0x18, 23, 3);
	clks[IMX6SL_CLK_SPDIF0_PRED]       = imx_clk_divider("spdif0_pred",       "spdif0_sel",        base + 0x30, 25, 3);
	clks[IMX6SL_CLK_SPDIF0_PODF]       = imx_clk_divider("spdif0_podf",       "spdif0_pred",       base + 0x30, 22, 3);
	clks[IMX6SL_CLK_SPDIF1_PRED]       = imx_clk_divider("spdif1_pred",       "spdif1_sel",        base + 0x30, 12, 3);
	clks[IMX6SL_CLK_SPDIF1_PODF]       = imx_clk_divider("spdif1_podf",       "spdif1_pred",       base + 0x30, 9,  3);
	clks[IMX6SL_CLK_EXTERN_AUDIO_PRED] = imx_clk_divider("extern_audio_pred", "extern_audio_sel",  base + 0x28, 9,  3);
	clks[IMX6SL_CLK_EXTERN_AUDIO_PODF] = imx_clk_divider("extern_audio_podf", "extern_audio_pred", base + 0x28, 25, 3);
	clks[IMX6SL_CLK_ECSPI_ROOT]        = imx_clk_divider("ecspi_root",        "ecspi_sel",         base + 0x38, 19, 6);
	clks[IMX6SL_CLK_UART_ROOT]         = imx_clk_divider("uart_root",         "uart_sel",          base + 0x24, 0,  6);

	/*                                                name         parent_name reg       shift width busy: reg, shift */
	clks[IMX6SL_CLK_AHB]       = imx_clk_busy_divider("ahb",       "periph",  base + 0x14, 10, 3,    base + 0x48, 1);
	clks[IMX6SL_CLK_MMDC_ROOT] = imx_clk_busy_divider("mmdc",      "periph2", base + 0x14, 3,  3,    base + 0x48, 2);
	clks[IMX6SL_CLK_ARM]       = imx_clk_busy_divider("arm",       "pll1_sw", base + 0x10, 0,  3,    base + 0x48, 16);

	/*                                            name            parent_name          reg         shift */
	clks[IMX6SL_CLK_ECSPI1]       = imx_clk_gate2("ecspi1",       "ecspi_root",        base + 0x6c, 0);
	clks[IMX6SL_CLK_ECSPI2]       = imx_clk_gate2("ecspi2",       "ecspi_root",        base + 0x6c, 2);
	clks[IMX6SL_CLK_ECSPI3]       = imx_clk_gate2("ecspi3",       "ecspi_root",        base + 0x6c, 4);
	clks[IMX6SL_CLK_ECSPI4]       = imx_clk_gate2("ecspi4",       "ecspi_root",        base + 0x6c, 6);
	clks[IMX6SL_CLK_ENET]         = imx_clk_gate2("enet",         "ipg",               base + 0x6c, 10);
	clks[IMX6SL_CLK_EPIT1]        = imx_clk_gate2("epit1",        "perclk",            base + 0x6c, 12);
	clks[IMX6SL_CLK_EPIT2]        = imx_clk_gate2("epit2",        "perclk",            base + 0x6c, 14);
	clks[IMX6SL_CLK_EXTERN_AUDIO] = imx_clk_gate2("extern_audio", "extern_audio_podf", base + 0x6c, 16);
	clks[IMX6SL_CLK_GPT]          = imx_clk_gate2("gpt",          "perclk",            base + 0x6c, 20);
	clks[IMX6SL_CLK_GPT_SERIAL]   = imx_clk_gate2("gpt_serial",   "perclk",            base + 0x6c, 22);
	clks[IMX6SL_CLK_GPU2D_OVG]    = imx_clk_gate2("gpu2d_ovg",    "gpu2d_ovg_podf",    base + 0x6c, 26);
	clks[IMX6SL_CLK_I2C1]         = imx_clk_gate2("i2c1",         "perclk",            base + 0x70, 6);
	clks[IMX6SL_CLK_I2C2]         = imx_clk_gate2("i2c2",         "perclk",            base + 0x70, 8);
	clks[IMX6SL_CLK_I2C3]         = imx_clk_gate2("i2c3",         "perclk",            base + 0x70, 10);
	clks[IMX6SL_CLK_OCOTP]        = imx_clk_gate2("ocotp",        "ipg",               base + 0x70, 12);
	clks[IMX6SL_CLK_CSI]          = imx_clk_gate2_flags("csi",          "csi_podf",          base + 0x74, 0, CLK_SET_RATE_PARENT);
	clks[IMX6SL_CLK_PXP_AXI]      = imx_clk_gate2("pxp_axi",      "pxp_axi_podf",      base + 0x74, 2);
	clks[IMX6SL_CLK_EPDC_AXI]     = imx_clk_gate2("epdc_axi",     "epdc_axi_podf",     base + 0x74, 4);
	clks[IMX6SL_CLK_LCDIF_AXI]    = imx_clk_gate2("lcdif_axi",    "lcdif_axi_podf",    base + 0x74, 6);
	clks[IMX6SL_CLK_LCDIF_PIX]    = imx_clk_gate2("lcdif_pix",    "lcdif_pix_podf",    base + 0x74, 8);
	clks[IMX6SL_CLK_EPDC_PIX]     = imx_clk_gate2("epdc_pix",     "epdc_pix_podf",     base + 0x74, 10);
	clks[IMX6SL_CLK_OCRAM]        = imx_clk_busy_gate("ocram",        "ocram_podf",        base + 0x74, 28);
	clks[IMX6SL_CLK_PWM1]         = imx_clk_gate2("pwm1",         "perclk",            base + 0x78, 16);
	clks[IMX6SL_CLK_PWM2]         = imx_clk_gate2("pwm2",         "perclk",            base + 0x78, 18);
	clks[IMX6SL_CLK_PWM3]         = imx_clk_gate2("pwm3",         "perclk",            base + 0x78, 20);
	clks[IMX6SL_CLK_PWM4]         = imx_clk_gate2("pwm4",         "perclk",            base + 0x78, 22);
	clks[IMX6SL_CLK_SDMA]         = imx_clk_gate2("sdma",         "ipg",               base + 0x7c, 6);
	clks[IMX6SL_CLK_SPBA]         = imx_clk_gate2("spba",         "ipg",               base + 0x7c, 12);
	clks[IMX6SL_CLK_SPDIF]        = imx_clk_gate2("spdif",        "spdif0_podf",       base + 0x7c, 14);
	clks[IMX6SL_CLK_SSI1]         = imx_clk_gate2("ssi1",         "ssi1_podf",         base + 0x7c, 18);
	clks[IMX6SL_CLK_SSI2]         = imx_clk_gate2("ssi2",         "ssi2_podf",         base + 0x7c, 20);
	clks[IMX6SL_CLK_SSI3]         = imx_clk_gate2("ssi3",         "ssi3_podf",         base + 0x7c, 22);
	clks[IMX6SL_CLK_UART]         = imx_clk_gate2("uart",         "ipg",               base + 0x7c, 24);
	clks[IMX6SL_CLK_UART_SERIAL]  = imx_clk_gate2("uart_serial",  "uart_root",         base + 0x7c, 26);
	clks[IMX6SL_CLK_USBOH3]       = imx_clk_gate2("usboh3",       "ipg",               base + 0x80, 0);
	clks[IMX6SL_CLK_USDHC1]       = imx_clk_gate2("usdhc1",       "usdhc1_podf",       base + 0x80, 2);
	clks[IMX6SL_CLK_USDHC2]       = imx_clk_gate2("usdhc2",       "usdhc2_podf",       base + 0x80, 4);
	clks[IMX6SL_CLK_USDHC3]       = imx_clk_gate2("usdhc3",       "usdhc3_podf",       base + 0x80, 6);
	clks[IMX6SL_CLK_USDHC4]       = imx_clk_gate2("usdhc4",       "usdhc4_podf",       base + 0x80, 8);

	for (i = 0; i < ARRAY_SIZE(clks); i++)
		if (IS_ERR(clks[i]))
			pr_err("i.MX6SL clk %d: register failed with %ld\n",
				i, PTR_ERR(clks[i]));

	/* Initialize clock gate status */
	writel_relaxed(1 << CCM_CCGR_OFFSET(11) |
		3 << CCM_CCGR_OFFSET(1) |
		3 << CCM_CCGR_OFFSET(0), base + 0x68);
	writel_relaxed(3 << CCM_CCGR_OFFSET(10), base + 0x6c);
	writel_relaxed(1 << CCM_CCGR_OFFSET(11) |
		3 << CCM_CCGR_OFFSET(10) |
		3 << CCM_CCGR_OFFSET(9) |
		3 << CCM_CCGR_OFFSET(8), base + 0x70);
	writel_relaxed(3 << CCM_CCGR_OFFSET(13) |
		3 << CCM_CCGR_OFFSET(12) |
		3 << CCM_CCGR_OFFSET(11) |
		3 << CCM_CCGR_OFFSET(10), base + 0x74);
	writel_relaxed(3 << CCM_CCGR_OFFSET(7) |
		3 << CCM_CCGR_OFFSET(4), base + 0x78);
	writel_relaxed(1 << CCM_CCGR_OFFSET(0), base + 0x7c);
	writel_relaxed(0, base + 0x80);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);

	clk_register_clkdev(clks[IMX6SL_CLK_GPT], "ipg", "imx-gpt.0");
	clk_register_clkdev(clks[IMX6SL_CLK_GPT_SERIAL], "per", "imx-gpt.0");

	/* Ensure the AHB clk is at 132MHz. */
	imx_clk_set_rate(clks[IMX6SL_CLK_AHB], 132000000);

	imx_clk_set_parent(clks[IMX6SL_CLK_GPU2D_OVG_SEL],
		clks[IMX6SL_CLK_PLL2_BUS]);
	imx_clk_set_parent(clks[IMX6SL_CLK_GPU2D_SEL], clks[IMX6SL_CLK_PLL2_BUS]);

	/* Initialize Video PLLs to valid frequency (650MHz). */
	imx_clk_set_rate(clks[IMX6SL_CLK_PLL5_VIDEO], 650000000);
	/* set PLL5 video as lcdif pix parent clock */
	imx_clk_set_parent(clks[IMX6SL_CLK_LCDIF_PIX_SEL],
			clks[IMX6SL_CLK_PLL5_VIDEO_DIV]);
	imx_clk_set_parent(clks[IMX6SL_CLK_EPDC_PIX_SEL],
			clks[IMX6SL_CLK_PLL2_PFD1]);

	imx_clk_set_parent(clks[IMX6SL_CLK_EPDC_AXI_SEL], clks[IMX6SL_CLK_PLL2_PFD2]);
	imx_clk_set_rate(clks[IMX6SL_CLK_EPDC_AXI], 200000000);
	imx_clk_set_parent(clks[IMX6SL_CLK_PXP_AXI_SEL], clks[IMX6SL_CLK_PLL2_PFD2]);
	imx_clk_set_rate(clks[IMX6SL_CLK_PXP_AXI], 200000000);
	imx_clk_set_parent(clks[IMX6SL_CLK_LCDIF_AXI_SEL], clks[IMX6SL_CLK_PLL2_PFD2]);
	imx_clk_set_rate(clks[IMX6SL_CLK_LCDIF_AXI], 200000000);

	/* Audio clocks */
	imx_clk_set_parent(clks[IMX6SL_CLK_SPDIF0_SEL], clks[IMX6SL_CLK_PLL3_PFD3]);
	imx_clk_set_rate(clks[IMX6SL_CLK_SPDIF0_PODF], 227368421);

	/* set extern_audio to be sourced from PLL4/audio PLL */
	imx_clk_set_parent(clks[IMX6SL_CLK_EXTERN_AUDIO_SEL], clks[IMX6SL_CLK_PLL4_AUDIO_DIV]);
	/* set extern_audio to 24MHz */
	imx_clk_set_rate(clks[IMX6SL_CLK_PLL4_AUDIO], 24000000);
	imx_clk_set_rate(clks[IMX6SL_CLK_EXTERN_AUDIO], 24000000);

	/* set SSI2 parent to PLL4 */
	imx_clk_set_parent(clks[IMX6SL_CLK_SSI2_SEL], clks[IMX6SL_CLK_PLL4_AUDIO_DIV]);
	imx_clk_set_rate(clks[IMX6SL_CLK_SSI2], 24000000);

	/* set perclk to source from OSC 24MHz */
	imx_clk_set_parent(clks[IMX6SL_CLK_PERCLK_SEL], clks[IMX6SL_CLK_OSC]);

	/* Set initial power mode */
	imx6_set_lpm(WAIT_CLOCKED);

	/* Ensure that CH0 handshake is bypassed. */
	reg = readl_relaxed(base + CCM_CCDR_OFFSET);
	reg |= 1 << CCDR_CH0_HS_BYP;
	writel_relaxed(reg, base + CCM_CCDR_OFFSET);

	/* Set the UART parent if needed. */
	if (uart_from_osc)
		imx_clk_set_parent(clks[IMX6SL_CLK_UART_SEL], clks[IMX6SL_CLK_UART_OSC_4M]);

	/*
	 * Enable clocks only after both parent and rate are all initialized
	 * as needed
	 */

	/*
	 * To prevent the bus clock from being disabled accidently when
	 * clk_disable() gets called on child clock, let's increment the use
	 * count of IPG clock by initially calling clk_prepare_enable() on it.
	 */
	imx_clk_prepare_enable(clks[IMX6SL_CLK_IPG]);

	/*
	 * Make sure the ARM clk is enabled to maintain the correct usecount
	 * and enabling/disabling of parent PLLs.
	 */
	imx_clk_prepare_enable(clks[IMX6SL_CLK_ARM]);

	/*
	 * Make sure the MMDC clk is enabled to maintain the correct usecount
	 * and enabling/disabling of parent PLLs.
	 */
	imx_clk_prepare_enable(clks[IMX6SL_CLK_MMDC_ROOT]);

	/*
	 * Make sure the OCRAM clk is enabled to maintain the correct usecount
	 * and enabling/disabling of parent PLLs.
	 */
	imx_clk_prepare_enable(clks[IMX6SL_CLK_OCRAM]);

	if (IS_ENABLED(CONFIG_USB_MXS_PHY)) {
		imx_clk_prepare_enable(clks[IMX6SL_CLK_USBPHY1_GATE]);
		imx_clk_prepare_enable(clks[IMX6SL_CLK_USBPHY2_GATE]);
	}

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6sl-gpt");
	base = of_iomap(np, 0);
	WARN_ON(!base);
	irq = irq_of_parse_and_map(np, 0);
	mxc_timer_init(base, irq);
}
CLK_OF_DECLARE(imx6sl, "fsl,imx6sl-ccm", imx6sl_clocks_init);
