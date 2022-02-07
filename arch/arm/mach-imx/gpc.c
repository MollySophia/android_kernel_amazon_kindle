/*
 * Copyright 2011-2014 Freescale Semiconductor, Inc.
 * Copyright 2011 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include "common.h"
#include "hardware.h"

#define GPC_IMR1		0x008
#define GPC_PGC_MF_PDN		0x220
#define GPC_PGC_CPU_PDN		0x2a0
#define GPC_PGC_GPU_PDN		0x260
#define GPC_PGC_GPU_PUPSCR	0x264
#define GPC_PGC_GPU_PDNSCR	0x268
#define GPC_PGC_DISP_PGCR_OFFSET	0x240
#define GPC_PGC_DISP_PUPSCR_OFFSET	0x244
#define GPC_PGC_DISP_PDNSCR_OFFSET	0x248
#define GPC_PGC_DISP_SR_OFFSET		0x24c
#define GPC_PGC_GPU_SW_SHIFT		0
#define GPC_PGC_GPU_SW_MASK		0x3f
#define GPC_PGC_GPU_SW2ISO_SHIFT	8
#define GPC_PGC_GPU_SW2ISO_MASK		0x3f
#define GPC_PGC_CPU_PUPSCR	0x2a4
#define GPC_PGC_CPU_PDNSCR	0x2a8
#define GPC_PGC_CPU_SW_SHIFT		0
#define GPC_PGC_CPU_SW_MASK		0x3f
#define GPC_PGC_CPU_SW2ISO_SHIFT	8
#define GPC_PGC_CPU_SW2ISO_MASK		0x3f
#define GPC_CNTR		0x0
#define GPC_CNTR_PCIE_PHY_PDU_SHIFT	0x7
#define GPC_CNTR_PCIE_PHY_PDN_SHIFT	0x6
#define PGC_PCIE_PHY_CTRL		0x200
#define PGC_PCIE_PHY_PDN_EN		0x1
#define GPC_CNTR_PU_UP_REQ_SHIFT	0x1
#define GPC_CNTR_PU_DOWN_REQ_SHIFT	0x0
#define GPC_M4_LPSR			0x2c
#define GPC_M4_LPSR_M4_SLEEPING_SHIFT	4
#define GPC_M4_LPSR_M4_SLEEPING_MASK	0x1
#define GPC_M4_LPSR_M4_SLEEP_HOLD_REQ_MASK	0x1
#define GPC_M4_LPSR_M4_SLEEP_HOLD_REQ_SHIFT	0
#define GPC_M4_LPSR_M4_SLEEP_HOLD_ACK_MASK	0x1
#define GPC_M4_LPSR_M4_SLEEP_HOLD_ACK_SHIFT	1

#define IMR_NUM			4

static DEFINE_SPINLOCK(gpc_lock);
static void __iomem *gpc_base;
static u32 gpc_mf_irqs[IMR_NUM];
static u32 gpc_wake_irqs[IMR_NUM];
static u32 gpc_saved_imrs[IMR_NUM];
static struct clk *gpu3d_clk, *gpu3d_shader_clk, *gpu2d_clk, *gpu2d_axi_clk;
static struct clk *lcd_axi_clk, *lcd_pix_clk, *epdc_axi_clk, *epdc_pix_clk;
static struct clk *pxp_axi_clk;
static struct clk *disp_axi_clk, *lcdif_axi_clk, *lcdif1_pix_clk, *lcdif2_pix_clk, *csi_mclk;
static struct clk *openvg_axi_clk, *vpu_clk, *ipg_clk;
static struct device *gpc_dev;
static struct regulator *pu_reg;
static struct notifier_block nb;
static struct notifier_block nb_pcie;
static struct regulator_dev *pu_dummy_regulator_rdev, *disp_regulator_rdev;
static struct regulator_init_data pu_dummy_initdata = {
	.constraints = {
		.max_uV = 1450000,	/* allign with real max of anatop */
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_VOLTAGE,
	},
};
static struct regulator_init_data dispreg_initdata = {
	.constraints = {
		.max_uV = 0, /* anyvalue */
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_VOLTAGE,
	},
};

static int pu_dummy_enable;
static int dispreg_enable;

static void imx_disp_clk(bool enable)
{
	if (cpu_is_imx6sl()) {
		if (enable) {
			clk_prepare_enable(lcd_axi_clk);
			clk_prepare_enable(lcd_pix_clk);
			clk_prepare_enable(epdc_axi_clk);
			clk_prepare_enable(epdc_pix_clk);
			clk_prepare_enable(pxp_axi_clk);
		} else {
			clk_disable_unprepare(lcd_axi_clk);
			clk_disable_unprepare(lcd_pix_clk);
			clk_disable_unprepare(epdc_axi_clk);
			clk_disable_unprepare(epdc_pix_clk);
			clk_disable_unprepare(pxp_axi_clk);
		}
	} else if (cpu_is_imx6sx()) {
		if (enable) {
			clk_prepare_enable(lcdif_axi_clk);
			clk_prepare_enable(lcdif1_pix_clk);
			clk_prepare_enable(lcdif2_pix_clk);
			clk_prepare_enable(pxp_axi_clk);
			clk_prepare_enable(csi_mclk);
			clk_prepare_enable(disp_axi_clk);
		} else {
			clk_disable_unprepare(lcdif_axi_clk);
			clk_disable_unprepare(lcdif1_pix_clk);
			clk_disable_unprepare(lcdif2_pix_clk);
			clk_disable_unprepare(pxp_axi_clk);
			clk_disable_unprepare(csi_mclk);
			clk_disable_unprepare(disp_axi_clk);
		}
	}
}

static void imx_gpc_dispmix_on(void)
{
	u32 val = readl_relaxed(gpc_base + GPC_CNTR);

	if ((cpu_is_imx6sl() &&
		imx_get_soc_revision() >= IMX_CHIP_REVISION_1_2) || cpu_is_imx6sx()) {
		imx_disp_clk(true);

		writel_relaxed(0x0, gpc_base + GPC_PGC_DISP_PGCR_OFFSET);
		writel_relaxed(0x20 | val, gpc_base + GPC_CNTR);
		while (readl_relaxed(gpc_base + GPC_CNTR) & 0x20)
			;
		writel_relaxed(0x1, gpc_base + GPC_PGC_DISP_SR_OFFSET);

		imx_disp_clk(false);
	}
}

static void imx_gpc_dispmix_off(void)
{
	u32 val = readl_relaxed(gpc_base + GPC_CNTR);

	if ((cpu_is_imx6sl() &&
		imx_get_soc_revision() >= IMX_CHIP_REVISION_1_2) || cpu_is_imx6sx()) {
		imx_disp_clk(true);

		writel_relaxed(0xFFFFFFFF,
				gpc_base + GPC_PGC_DISP_PUPSCR_OFFSET);
		writel_relaxed(0xFFFFFFFF,
				gpc_base + GPC_PGC_DISP_PDNSCR_OFFSET);
		writel_relaxed(0x1, gpc_base + GPC_PGC_DISP_PGCR_OFFSET);
		writel_relaxed(0x10 | val, gpc_base + GPC_CNTR);
		while (readl_relaxed(gpc_base + GPC_CNTR) & 0x10)
			;

		imx_disp_clk(false);
	}
}

void imx_gpc_add_m4_wake_up_irq(u32 irq, bool enable)
{
	unsigned int idx = irq / 32 - 1;
	unsigned long flags;
	u32 mask;

	/* Sanity check for SPI irq */
	if (irq < 32)
		return;

	mask = 1 << irq % 32;
	spin_lock_irqsave(&gpc_lock, flags);
	gpc_wake_irqs[idx] = enable ? gpc_wake_irqs[idx] | mask :
				  gpc_wake_irqs[idx] & ~mask;
	spin_unlock_irqrestore(&gpc_lock, flags);
}

unsigned int imx_gpc_is_m4_sleeping(void)
{
	if (readl_relaxed(gpc_base + GPC_M4_LPSR) &
		(GPC_M4_LPSR_M4_SLEEPING_MASK <<
		GPC_M4_LPSR_M4_SLEEPING_SHIFT))
		return 1;

	return 0;
}

unsigned int imx_gpc_is_mf_mix_off(void)
{
	return readl_relaxed(gpc_base + GPC_PGC_MF_PDN);
}

static void imx_gpc_mf_mix_off(void)
{
	int i;

	for (i = 0; i < IMR_NUM; i++)
		if ((gpc_wake_irqs[i] & gpc_mf_irqs[i]) != 0)
			return;

	pr_info("Turn off M/F mix!\n");
	/* turn off mega/fast mix */
	writel_relaxed(0x1, gpc_base + GPC_PGC_MF_PDN);
}

void imx_gpc_pre_suspend(bool arm_power_off)
{
	void __iomem *reg_imr1 = gpc_base + GPC_IMR1;
	int i;

	imx_gpc_dispmix_off();

	if (cpu_is_imx6sx() && arm_power_off)
		imx_gpc_mf_mix_off();

	if (arm_power_off)
		/* Tell GPC to power off ARM core when suspend */
		writel_relaxed(0x1, gpc_base + GPC_PGC_CPU_PDN);

	for (i = 0; i < IMR_NUM; i++) {
		gpc_saved_imrs[i] = readl_relaxed(reg_imr1 + i * 4);
		writel_relaxed(~gpc_wake_irqs[i], reg_imr1 + i * 4);
	}
}

void imx_gpc_hold_m4_in_sleep()
{
	int val;
	unsigned long timeout = jiffies + msecs_to_jiffies(500);

	/* wait M4 in wfi before asserting hold request */
	while (!imx_gpc_is_m4_sleeping())
		if (time_after(jiffies, timeout))
			pr_err("M4 is NOT in expected sleep!\n");

	val = readl_relaxed(gpc_base + GPC_M4_LPSR);
	val &= ~(GPC_M4_LPSR_M4_SLEEP_HOLD_REQ_MASK <<
		GPC_M4_LPSR_M4_SLEEP_HOLD_REQ_SHIFT);
	writel_relaxed(val, gpc_base + GPC_M4_LPSR);

	timeout = jiffies + msecs_to_jiffies(500);
	while (readl_relaxed(gpc_base + GPC_M4_LPSR)
		& (GPC_M4_LPSR_M4_SLEEP_HOLD_ACK_MASK <<
		GPC_M4_LPSR_M4_SLEEP_HOLD_ACK_SHIFT))
		if (time_after(jiffies, timeout))
			pr_err("Wait M4 hold ack timeout!\n");
}

void imx_gpc_release_m4_in_sleep()
{
	int val;

	val = readl_relaxed(gpc_base + GPC_M4_LPSR);
	val |= GPC_M4_LPSR_M4_SLEEP_HOLD_REQ_MASK <<
		GPC_M4_LPSR_M4_SLEEP_HOLD_REQ_SHIFT;
	writel_relaxed(val, gpc_base + GPC_M4_LPSR);
}

void imx_gpc_post_resume(void)
{
	void __iomem *reg_imr1 = gpc_base + GPC_IMR1;
	int i;

	/* Keep ARM core powered on for other low-power modes */
	writel_relaxed(0x0, gpc_base + GPC_PGC_CPU_PDN);

	/* Keep M/F mix powered on for other low-power modes */
	if (cpu_is_imx6sx())
		writel_relaxed(0x0, gpc_base + GPC_PGC_MF_PDN);

	for (i = 0; i < IMR_NUM; i++)
		writel_relaxed(gpc_saved_imrs[i], reg_imr1 + i * 4);

	imx_gpc_dispmix_on();
}

static int imx_gpc_irq_set_wake(struct irq_data *d, unsigned int on)
{
	unsigned int idx = d->irq / 32 - 1;
	unsigned long flags;
	u32 mask;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return -EINVAL;

	mask = 1 << d->irq % 32;
	spin_lock_irqsave(&gpc_lock, flags);
	gpc_wake_irqs[idx] = on ? gpc_wake_irqs[idx] | mask :
				  gpc_wake_irqs[idx] & ~mask;
	spin_unlock_irqrestore(&gpc_lock, flags);

	return 0;
}

void imx_gpc_mask_all(void)
{
	void __iomem *reg_imr1 = gpc_base + GPC_IMR1;
	int i;

	for (i = 0; i < IMR_NUM; i++) {
		gpc_saved_imrs[i] = readl_relaxed(reg_imr1 + i * 4);
		writel_relaxed(~0, reg_imr1 + i * 4);
	}

}

void imx_gpc_restore_all(void)
{
	void __iomem *reg_imr1 = gpc_base + GPC_IMR1;
	int i;

	for (i = 0; i < IMR_NUM; i++)
		writel_relaxed(gpc_saved_imrs[i], reg_imr1 + i * 4);
}

void imx_gpc_irq_unmask(struct irq_data *d)
{
	void __iomem *reg;
	u32 val;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return;

	reg = gpc_base + GPC_IMR1 + (d->irq / 32 - 1) * 4;
	val = readl_relaxed(reg);
	val &= ~(1 << d->irq % 32);
	writel_relaxed(val, reg);
}

void imx_gpc_irq_mask(struct irq_data *d)
{
	void __iomem *reg;
	u32 val;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return;

	reg = gpc_base + GPC_IMR1 + (d->irq / 32 - 1) * 4;
	val = readl_relaxed(reg);
	val |= 1 << (d->irq % 32);
	writel_relaxed(val, reg);
}

static void imx_pu_clk(bool enable)
{
	if (enable) {
		if (cpu_is_imx6sl()) {
			clk_prepare_enable(gpu2d_clk);
			clk_prepare_enable(openvg_axi_clk);
		} else if (cpu_is_imx6sx()) {
			clk_prepare_enable(gpu3d_clk);
		} else {
			clk_prepare_enable(gpu3d_clk);
			clk_prepare_enable(gpu3d_shader_clk);
			clk_prepare_enable(vpu_clk);
			clk_prepare_enable(gpu2d_clk);
			clk_prepare_enable(gpu2d_axi_clk);
			clk_prepare_enable(openvg_axi_clk);
		}
	} else {
		if (cpu_is_imx6sl()) {
			clk_disable_unprepare(gpu2d_clk);
			clk_disable_unprepare(openvg_axi_clk);
		} else if (cpu_is_imx6sx()) {
			clk_disable_unprepare(gpu3d_clk);
		} else {
			clk_disable_unprepare(gpu3d_clk);
			clk_disable_unprepare(gpu3d_shader_clk);
			clk_disable_unprepare(vpu_clk);
			clk_disable_unprepare(gpu2d_clk);
			clk_disable_unprepare(gpu2d_axi_clk);
			clk_disable_unprepare(openvg_axi_clk);
		}
	}
}

static void imx_gpc_pu_enable(bool enable)
{
	u32 rate, delay_us;
	u32 gpu_pupscr_sw2iso, gpu_pdnscr_iso2sw;
	u32 gpu_pupscr_sw, gpu_pdnscr_iso;

	/* get ipg clk rate for PGC delay */
	rate = clk_get_rate(ipg_clk);

	if (enable) {
		imx_anatop_pu_enable(true);
		/*
		 * need to add necessary delay between powering up PU LDO and
		 * disabling PU isolation in PGC, the counter of PU isolation
		 * is based on ipg clk.
		 */
		gpu_pupscr_sw2iso = (readl_relaxed(gpc_base +
			GPC_PGC_GPU_PUPSCR) >> GPC_PGC_GPU_SW2ISO_SHIFT)
			& GPC_PGC_GPU_SW2ISO_MASK;
		gpu_pupscr_sw = (readl_relaxed(gpc_base +
			GPC_PGC_GPU_PUPSCR) >> GPC_PGC_GPU_SW_SHIFT)
			& GPC_PGC_GPU_SW_MASK;
		delay_us = (gpu_pupscr_sw2iso + gpu_pupscr_sw) * 1000000
			/ rate + 1;
		udelay(delay_us);

		imx_pu_clk(true);
		writel_relaxed(1, gpc_base + GPC_PGC_GPU_PDN);
		/*
		 * bit17 and bit18 as VADC power state control are different
		 * as the other bits in GPC_CNTR  whose request is set by
		 * 1 and nothing involved if set by 0. On imx6sx, zero of bit
		 * 17 and bit18 will power off VADC directly, so read GPC_CNTR
		 * firstly before write to avoid touching other bits.
		 */
		if (cpu_is_imx6sx()) {
			u32 value = readl_relaxed(gpc_base + GPC_CNTR);

			value |= 1 << GPC_CNTR_PU_UP_REQ_SHIFT;
			writel_relaxed(value, gpc_base + GPC_CNTR);
		} else
			writel_relaxed(1 << GPC_CNTR_PU_UP_REQ_SHIFT,
				gpc_base + GPC_CNTR);
		while (readl_relaxed(gpc_base + GPC_CNTR) &
			(1 << GPC_CNTR_PU_UP_REQ_SHIFT))
			;
		imx_pu_clk(false);
	} else {
		writel_relaxed(1, gpc_base + GPC_PGC_GPU_PDN);
		/*
		 * bit17 and bit18 as VADC power state control are different
		 * as the other bits in GPC_CNTR  whose request is set by
		 * 1 and nothing involved if set by 0. On imx6sx, zero of bit
		 * 17 and bit18 will power off VADC directly, so read GPC_CNTR
		 * firstly before write to avoid touching other bits.
		 */
		if (cpu_is_imx6sx()) {
			u32 value = readl_relaxed(gpc_base + GPC_CNTR);

			value |= 1 << GPC_CNTR_PU_DOWN_REQ_SHIFT;
			writel_relaxed(value, gpc_base + GPC_CNTR);
		} else
			writel_relaxed(1 << GPC_CNTR_PU_DOWN_REQ_SHIFT,
					gpc_base + GPC_CNTR);
		while (readl_relaxed(gpc_base + GPC_CNTR) &
			(1 << GPC_CNTR_PU_DOWN_REQ_SHIFT))
			;
		/*
		 * need to add necessary delay between enabling PU isolation
		 * in PGC and powering down PU LDO , the counter of PU isolation
		 * is based on ipg clk.
		 */
		gpu_pdnscr_iso2sw = (readl_relaxed(gpc_base +
			GPC_PGC_GPU_PDNSCR) >> GPC_PGC_GPU_SW2ISO_SHIFT)
			& GPC_PGC_GPU_SW2ISO_MASK;
		gpu_pdnscr_iso = (readl_relaxed(gpc_base +
			GPC_PGC_GPU_PDNSCR) >> GPC_PGC_GPU_SW_SHIFT)
			& GPC_PGC_GPU_SW_MASK;
		delay_us = (gpu_pdnscr_iso2sw + gpu_pdnscr_iso) * 1000000
			/ rate + 1;
		udelay(delay_us);
		imx_anatop_pu_enable(false);
	}
}

static int imx_gpc_regulator_notify(struct notifier_block *nb,
					unsigned long event,
					void *ignored)
{
	switch (event) {
	case REGULATOR_EVENT_PRE_DISABLE:
		imx_gpc_pu_enable(false);
		break;
	case REGULATOR_EVENT_ENABLE:
		imx_gpc_pu_enable(true);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int imx_pcie_regulator_notify(struct notifier_block *nb,
					unsigned long event,
					void *ignored)
{
	u32 value = readl_relaxed(gpc_base + GPC_CNTR);

	switch (event) {
	case REGULATOR_EVENT_VOLTAGE_CHANGE:
	case REGULATOR_EVENT_ENABLE:
		value |= 1 << GPC_CNTR_PCIE_PHY_PDU_SHIFT;
		writel_relaxed(value, gpc_base + GPC_CNTR);
		break;
	case REGULATOR_EVENT_PRE_DISABLE:
		value |= 1 << GPC_CNTR_PCIE_PHY_PDN_SHIFT;
		writel_relaxed(value, gpc_base + GPC_CNTR);
		writel_relaxed(PGC_PCIE_PHY_PDN_EN,
				gpc_base + PGC_PCIE_PHY_CTRL);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

void __init imx_gpc_init(void)
{
	struct device_node *np;
	int i;
	u32 val;
	u32 cpu_pupscr_sw2iso, cpu_pupscr_sw;
	u32 cpu_pdnscr_iso2sw, cpu_pdnscr_iso;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-gpc");
	gpc_base = of_iomap(np, 0);
	WARN_ON(!gpc_base);

	/* Initially mask all interrupts */
	for (i = 0; i < IMR_NUM; i++)
		writel_relaxed(~0, gpc_base + GPC_IMR1 + i * 4);

	/* Register GPC as the secondary interrupt controller behind GIC */
	gic_arch_extn.irq_mask = imx_gpc_irq_mask;
	gic_arch_extn.irq_unmask = imx_gpc_irq_unmask;
	gic_arch_extn.irq_set_wake = imx_gpc_irq_set_wake;

	/*
	 * If there are CPU isolation timing settings in dts,
	 * update them according to dts, otherwise, keep them
	 * with default value in registers.
	 */
	cpu_pupscr_sw2iso = cpu_pupscr_sw =
		cpu_pdnscr_iso2sw = cpu_pdnscr_iso = 0;

	/* Read CPU isolation setting for GPC */
	of_property_read_u32(np, "fsl,cpu_pupscr_sw2iso", &cpu_pupscr_sw2iso);
	of_property_read_u32(np, "fsl,cpu_pupscr_sw", &cpu_pupscr_sw);
	of_property_read_u32(np, "fsl,cpu_pdnscr_iso2sw", &cpu_pdnscr_iso2sw);
	of_property_read_u32(np, "fsl,cpu_pdnscr_iso", &cpu_pdnscr_iso);

	/* Read supported wakeup source in M/F domain */
	if (cpu_is_imx6sx()) {
		of_property_read_u32_index(np, "fsl,mf-mix-wakeup-irq", 0,
			&gpc_mf_irqs[0]);
		of_property_read_u32_index(np, "fsl,mf-mix-wakeup-irq", 1,
			&gpc_mf_irqs[1]);
		of_property_read_u32_index(np, "fsl,mf-mix-wakeup-irq", 2,
			&gpc_mf_irqs[2]);
		of_property_read_u32_index(np, "fsl,mf-mix-wakeup-irq", 3,
			&gpc_mf_irqs[3]);
		if (!(gpc_mf_irqs[0] | gpc_mf_irqs[1] |
			gpc_mf_irqs[2] | gpc_mf_irqs[3]))
			pr_info("No wakeup source in Mega/Fast domain found!\n");
	}

	/* Update CPU PUPSCR timing if it is defined in dts */
	val = readl_relaxed(gpc_base + GPC_PGC_CPU_PUPSCR);
	if (cpu_pupscr_sw2iso)
		val &= ~(GPC_PGC_CPU_SW2ISO_MASK << GPC_PGC_CPU_SW2ISO_SHIFT);
	if (cpu_pupscr_sw)
		val &= ~(GPC_PGC_CPU_SW_MASK << GPC_PGC_CPU_SW_SHIFT);
	val |= cpu_pupscr_sw2iso << GPC_PGC_CPU_SW2ISO_SHIFT;
	val |= cpu_pupscr_sw << GPC_PGC_CPU_SW_SHIFT;
	writel_relaxed(val, gpc_base + GPC_PGC_CPU_PUPSCR);

	/* Update CPU PDNSCR timing if it is defined in dts */
	val = readl_relaxed(gpc_base + GPC_PGC_CPU_PDNSCR);
	if (cpu_pdnscr_iso2sw)
		val &= ~(GPC_PGC_CPU_SW2ISO_MASK << GPC_PGC_CPU_SW2ISO_SHIFT);
	if (cpu_pdnscr_iso)
		val &= ~(GPC_PGC_CPU_SW_MASK << GPC_PGC_CPU_SW_SHIFT);
	val |= cpu_pdnscr_iso2sw << GPC_PGC_CPU_SW2ISO_SHIFT;
	val |= cpu_pdnscr_iso << GPC_PGC_CPU_SW_SHIFT;
	writel_relaxed(val, gpc_base + GPC_PGC_CPU_PDNSCR);
}

static int imx_pureg_set_voltage(struct regulator_dev *reg, int min_uV,
					int max_uV, unsigned *selector)
{
	return 0;
}

static int imx_pureg_enable(struct regulator_dev *rdev)
{
	pu_dummy_enable = 1;

	return 0;
}

static int imx_pureg_disable(struct regulator_dev *rdev)
{
	pu_dummy_enable = 0;

	return 0;
}

static int imx_pureg_is_enable(struct regulator_dev *rdev)
{
	return pu_dummy_enable;
}

static int imx_pureg_list_voltage(struct regulator_dev *rdev,
				unsigned int selector)
{
	return 0;
}

static struct regulator_ops pu_dummy_ops = {
	.set_voltage = imx_pureg_set_voltage,
	.enable	= imx_pureg_enable,
	.disable = imx_pureg_disable,
	.is_enabled = imx_pureg_is_enable,
	.list_voltage = imx_pureg_list_voltage,
};

static struct regulator_desc pu_dummy_desc = {
	.name = "pureg-dummy",
	.id = -1,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
	.ops = &pu_dummy_ops,
};

static int pu_dummy_probe(struct platform_device *pdev)
{
	struct regulator_config config = { };
	int ret;

	config.dev = &pdev->dev;
	config.init_data = &pu_dummy_initdata;
	config.of_node = pdev->dev.of_node;

	pu_dummy_regulator_rdev = regulator_register(&pu_dummy_desc, &config);
	if (IS_ERR(pu_dummy_regulator_rdev)) {
		ret = PTR_ERR(pu_dummy_regulator_rdev);
		dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id imx_pudummy_ids[] = {
	{ .compatible = "fsl,imx6-dummy-pureg" },
};
MODULE_DEVICE_TABLE(of, imx_pudummy_ids);

static struct platform_driver pu_dummy_driver = {
	.probe	= pu_dummy_probe,
	.driver	= {
		.name	= "pu-dummy",
		.owner	= THIS_MODULE,
		.of_match_table = imx_pudummy_ids,
	},
};

static int imx_dispreg_enable(struct regulator_dev *rdev)
{
	imx_gpc_dispmix_on();
	dispreg_enable = 1;

	return 0;
}

static int imx_dispreg_disable(struct regulator_dev *rdev)
{
	imx_gpc_dispmix_off();
	dispreg_enable = 0;

	return 0;
}

static int imx_dispreg_is_enable(struct regulator_dev *rdev)
{
	return dispreg_enable;
}

static struct regulator_ops dispreg_ops = {
	.enable = imx_dispreg_enable,
	.disable = imx_dispreg_disable,
	.is_enabled = imx_dispreg_is_enable,
};

static struct regulator_desc dispreg_desc = {
	.name = "disp-regulator",
	.id = -1,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
	.ops = &dispreg_ops,
};

static int dispreg_probe(struct platform_device *pdev)
{
	struct regulator_config config = {};
	int ret = 0;

	config.dev = &pdev->dev;
	config.init_data = &dispreg_initdata;
	config.of_node = pdev->dev.of_node;

	disp_regulator_rdev = regulator_register(&dispreg_desc, &config);
	if (IS_ERR(disp_regulator_rdev)) {
		ret = PTR_ERR(disp_regulator_rdev);
		dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
	}

	return ret;
}

static const struct of_device_id imx_dispreg_ids[] = {
	{ .compatible = "fsl,imx6-display-regulator" },
	{}
};
MODULE_DEVICE_TABLE(of, imx_dispreg_ids);

static struct platform_driver dispreg_driver = {
	.probe  = dispreg_probe,
	.driver = {
		.name   = "disp-regulator",
		.owner  = THIS_MODULE,
		.of_match_table = imx_dispreg_ids,
	},
};

static int imx_gpc_probe(struct platform_device *pdev)
{
	int ret;

	gpc_dev = &pdev->dev;

	pu_reg = devm_regulator_get(gpc_dev, "pu");
	if (IS_ERR(pu_reg)) {
		ret = PTR_ERR(pu_reg);
		dev_info(gpc_dev, "pu regulator not ready.\n");
		return ret;
	}
	nb.notifier_call = &imx_gpc_regulator_notify;

	if (cpu_is_imx6sx()) {
		struct regulator *pcie_reg;

		pcie_reg = devm_regulator_get(gpc_dev, "pcie");
		if (IS_ERR(pcie_reg)) {
			ret = PTR_ERR(pcie_reg);
			dev_info(gpc_dev, "pcie regulator not ready.\n");
			return ret;
		}
		nb_pcie.notifier_call = &imx_pcie_regulator_notify;

		ret = regulator_register_notifier(pcie_reg, &nb_pcie);
		if (ret) {
			dev_err(gpc_dev,
				"pcie regulator notifier request failed\n");
			return ret;
		}
	}

	/* Get gpu&vpu clk for power up PU by GPC */
	if (cpu_is_imx6sl()) {
		gpu2d_clk = devm_clk_get(gpc_dev, "gpu2d_podf");
		openvg_axi_clk = devm_clk_get(gpc_dev, "gpu2d_ovg");
		ipg_clk = devm_clk_get(gpc_dev, "ipg");
		lcd_axi_clk = devm_clk_get(gpc_dev, "lcd_axi");
		lcd_pix_clk = devm_clk_get(gpc_dev, "lcd_pix");
		epdc_axi_clk = devm_clk_get(gpc_dev, "epdc_axi");
		epdc_pix_clk = devm_clk_get(gpc_dev, "epdc_pix");
		pxp_axi_clk = devm_clk_get(gpc_dev, "pxp_axi");
		if (IS_ERR(gpu2d_clk) || IS_ERR(openvg_axi_clk)
			|| IS_ERR(ipg_clk) || IS_ERR(lcd_axi_clk)
			|| IS_ERR(lcd_pix_clk) || IS_ERR(epdc_axi_clk)
			|| IS_ERR(epdc_pix_clk) || IS_ERR(pxp_axi_clk)) {
			dev_err(gpc_dev, "failed to get clk!\n");
			return -ENOENT;
		}
	} else if (cpu_is_imx6sx()) {
		gpu3d_clk = devm_clk_get(gpc_dev, "gpu3d_core");
		ipg_clk = devm_clk_get(gpc_dev, "ipg");
		pxp_axi_clk = devm_clk_get(gpc_dev, "pxp_axi");
		disp_axi_clk = devm_clk_get(gpc_dev, "disp_axi");
		lcdif1_pix_clk = devm_clk_get(gpc_dev, "lcdif1_pix");
		lcdif_axi_clk = devm_clk_get(gpc_dev, "lcdif_axi");
		lcdif2_pix_clk = devm_clk_get(gpc_dev, "lcdif2_pix");
		csi_mclk = devm_clk_get(gpc_dev, "csi_mclk");
		if (IS_ERR(gpu3d_clk) || IS_ERR(ipg_clk) || IS_ERR(pxp_axi_clk) || IS_ERR(disp_axi_clk) ||
			IS_ERR(lcdif1_pix_clk) || IS_ERR(lcdif_axi_clk) || IS_ERR(lcdif2_pix_clk) || IS_ERR(csi_mclk)) {
			dev_err(gpc_dev, "failed to get clk!\n");
			return -ENOENT;
		}
	} else {
		gpu3d_clk = devm_clk_get(gpc_dev, "gpu3d_core");
		gpu3d_shader_clk = devm_clk_get(gpc_dev, "gpu3d_shader");
		gpu2d_clk = devm_clk_get(gpc_dev, "gpu2d_core");
		gpu2d_axi_clk = devm_clk_get(gpc_dev, "gpu2d_axi");
		openvg_axi_clk = devm_clk_get(gpc_dev, "openvg_axi");
		vpu_clk = devm_clk_get(gpc_dev, "vpu_axi");
		ipg_clk = devm_clk_get(gpc_dev, "ipg");
		if (IS_ERR(gpu3d_clk) || IS_ERR(gpu3d_shader_clk)
			|| IS_ERR(gpu2d_clk) || IS_ERR(gpu2d_axi_clk)
			|| IS_ERR(openvg_axi_clk) || IS_ERR(vpu_clk)
			|| IS_ERR(ipg_clk)) {
			dev_err(gpc_dev, "failed to get clk!\n");
			return -ENOENT;
		}
	}

	ret = regulator_register_notifier(pu_reg, &nb);
	if (ret) {
		dev_err(gpc_dev,
			"regulator notifier request failed\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id imx_gpc_ids[] = {
	{ .compatible = "fsl,imx6q-gpc" },
};
MODULE_DEVICE_TABLE(of, imx_gpc_ids);

static struct platform_driver imx_gpc_platdrv = {
	.driver = {
		.name	= "imx-gpc",
		.owner	= THIS_MODULE,
		.of_match_table = imx_gpc_ids,
	},
	.probe		= imx_gpc_probe,
};

static int __init imx6_gpc_init(void)
{
	return platform_driver_probe(&imx_gpc_platdrv, imx_gpc_probe);
}
fs_initcall(imx6_gpc_init);

static int __init imx6_pudummy_init(void)
{
	return platform_driver_probe(&pu_dummy_driver, pu_dummy_probe);
}
fs_initcall(imx6_pudummy_init);

static int __init imx6_dispreg_init(void)
{
	return platform_driver_probe(&dispreg_driver, dispreg_probe);
}
fs_initcall(imx6_dispreg_init);

MODULE_AUTHOR("Anson Huang <b20788@freescale.com>");
MODULE_DESCRIPTION("Freescale i.MX GPC driver");
MODULE_LICENSE("GPL");
