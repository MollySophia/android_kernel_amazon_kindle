/*
 * Copyright 2004-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_COMMON_H__
#define __ASM_ARCH_MXC_COMMON_H__

struct platform_device;
struct pt_regs;
struct clk;
struct clk_hw;
struct irq_data;
enum mxc_cpu_pwr_mode;

extern void mx1_map_io(void);
extern void mx21_map_io(void);
extern void mx25_map_io(void);
extern void mx27_map_io(void);
extern void mx31_map_io(void);
extern void mx35_map_io(void);
extern void mx51_map_io(void);
extern void mx53_map_io(void);
extern void imx1_init_early(void);
extern void imx21_init_early(void);
extern void imx25_init_early(void);
extern void imx27_init_early(void);
extern void imx31_init_early(void);
extern void imx35_init_early(void);
extern void imx51_init_early(void);
extern void imx53_init_early(void);
extern void mxc_init_irq(void __iomem *);
extern void tzic_init_irq(void __iomem *);
extern void mx1_init_irq(void);
extern void mx21_init_irq(void);
extern void mx25_init_irq(void);
extern void mx27_init_irq(void);
extern void mx31_init_irq(void);
extern void mx35_init_irq(void);
extern void mx51_init_irq(void);
extern void mx53_init_irq(void);
extern void imx1_soc_init(void);
extern void imx21_soc_init(void);
extern void imx25_soc_init(void);
extern void imx27_soc_init(void);
extern void imx31_soc_init(void);
extern void imx35_soc_init(void);
extern void imx51_soc_init(void);
extern void imx51_init_late(void);
extern void imx53_init_late(void);
extern void epit_timer_init(void __iomem *base, int irq);
extern void mxc_timer_init(void __iomem *, int);
extern int mx1_clocks_init(unsigned long fref);
extern int mx21_clocks_init(unsigned long lref, unsigned long fref);
extern int mx25_clocks_init(void);
extern int mx27_clocks_init(unsigned long fref);
extern int mx31_clocks_init(unsigned long fref);
extern int mx35_clocks_init(void);
extern int mx51_clocks_init(unsigned long ckil, unsigned long osc,
			unsigned long ckih1, unsigned long ckih2);
extern int mx53_clocks_init(unsigned long ckil, unsigned long osc,
			unsigned long ckih1, unsigned long ckih2);
extern int mx25_clocks_init_dt(void);
extern int mx27_clocks_init_dt(void);
extern int mx31_clocks_init_dt(void);
extern int mx51_clocks_init_dt(void);
extern int mx53_clocks_init_dt(void);
extern struct platform_device *mxc_register_gpio(char *name, int id,
	resource_size_t iobase, resource_size_t iosize, int irq, int irq_high);
extern void mxc_set_cpu_type(unsigned int type);
extern void mxc_restart(char, const char *);
extern void mxc_arch_reset_init(void __iomem *);
extern void mxc_arch_reset_init_dt(void);
extern int mx53_revision(void);
extern int mx53_display_revision(void);
extern void imx_set_aips(void __iomem *);
extern int mxc_device_init(void);
extern void imx_set_soc_revision(unsigned int rev);
extern unsigned int imx_get_soc_revision(void);
extern void imx_init_revision_from_anatop(void);
extern struct device *imx_soc_device_init(void);
extern void imx6sx_low_power_idle(void);
extern void imx6_enable_rbc(bool enable);
extern unsigned int imx_gpc_is_mf_mix_off(void);
extern int imx_update_shared_mem(struct clk_hw *hw, bool enable);
extern bool imx_src_is_m4_enabled(void);
extern void mcc_receive_from_mu_buffer(unsigned int index, unsigned int *data);
extern void mcc_send_via_mu_buffer(unsigned int index, unsigned int data);

enum mxc_cpu_pwr_mode {
	WAIT_CLOCKED,		/* wfi only */
	WAIT_UNCLOCKED,		/* WAIT */
	WAIT_UNCLOCKED_POWER_OFF,	/* WAIT + SRPG */
	STOP_POWER_ON,		/* just STOP */
	STOP_POWER_OFF,		/* STOP + SRPG */
};

enum mx3_cpu_pwr_mode {
	MX3_RUN,
	MX3_WAIT,
	MX3_DOZE,
	MX3_SLEEP,
};

extern void mx3_cpu_lp_set(enum mx3_cpu_pwr_mode mode);
extern void imx_print_silicon_rev(const char *cpu, int srev);

void avic_handle_irq(struct pt_regs *);
void tzic_handle_irq(struct pt_regs *);

#define imx1_handle_irq avic_handle_irq
#define imx21_handle_irq avic_handle_irq
#define imx25_handle_irq avic_handle_irq
#define imx27_handle_irq avic_handle_irq
#define imx31_handle_irq avic_handle_irq
#define imx35_handle_irq avic_handle_irq
#define imx51_handle_irq tzic_handle_irq
#define imx53_handle_irq tzic_handle_irq

extern void imx_enable_cpu(int cpu, bool enable);
extern void imx_set_cpu_jump(int cpu, void *jump_addr);
extern u32 imx_get_cpu_arg(int cpu);
extern u32 imx_get_smbr1(void);
extern void imx_set_cpu_arg(int cpu, u32 arg);
extern void v7_cpu_resume(void);
#ifdef CONFIG_SMP
extern void v7_secondary_startup(void);
extern void imx_scu_map_io(void);
extern void imx_smp_prepare(void);
extern void imx_scu_standby_enable(void);
#else
static inline void imx_scu_map_io(void) {}
static inline void imx_smp_prepare(void) {}
static inline void imx_scu_standby_enable(void) {}
#endif
extern void imx6_pm_map_io(void);
extern void imx6_busfreq_map_io(void);
extern void imx6_suspend(void);
extern void imx_src_init(void);
#ifdef CONFIG_HAVE_IMX_SRC
extern void imx_src_prepare_restart(void);
#else
static inline void imx_src_prepare_restart(void) {}
#endif
extern void imx_gpc_init(void);
extern void imx_gpc_pre_suspend(bool arm_power_off);
extern void imx_gpc_post_resume(void);
extern void imx_gpc_mask_all(void);
extern void imx_gpc_irq_mask(struct irq_data *d);
extern void imx_gpc_irq_unmask(struct irq_data *d);
extern void imx_gpc_restore_all(void);
extern void imx_anatop_init(void);
extern void imx_anatop_pre_suspend(void);
extern void imx_anatop_post_resume(void);
extern void imx_anatop_pu_enable(bool enable);
extern int imx6_set_lpm(enum mxc_cpu_pwr_mode mode);
extern void imx6_set_cache_lpm_in_wait(bool enable);
extern void imx6sl_set_wait_clk(bool enter);
extern void imx6_enet_mac_init(const char *compatible);
extern int imx_mmdc_get_ddr_type(void);
extern unsigned int imx_gpc_is_m4_sleeping(void);
extern void imx_gpc_hold_m4_in_sleep(void);
extern void imx_gpc_release_m4_in_sleep(void);
extern void imx_gpc_add_m4_wake_up_irq(u32 irq, bool enable);
extern void mcc_enable_m4_irqs_in_gic(bool enable);
extern void imx6sx_set_m4_highfreq(bool high_freq);
extern void imx_cpu_die(unsigned int cpu);
extern int imx_cpu_kill(unsigned int cpu);

#ifdef CONFIG_PM
extern void imx6_pm_init(void);
extern void imx6_pm_set_ccm_base(void __iomem *base);
extern void imx51_pm_init(void);
extern void imx53_pm_init(void);
#else
static inline void imx6_pm_init(void) {}
static inline void imx6_pm_set_ccm_base(void __iomem *base) {}
static inline void imx51_pm_init(void) {}
static inline void imx53_pm_init(void) {}
#endif

#ifdef CONFIG_NEON
extern int mx51_neon_fixup(void);
#else
static inline int mx51_neon_fixup(void) { return 0; }
#endif

#ifdef CONFIG_CACHE_L2X0
extern void imx_init_l2cache(void);
#else
static inline void imx_init_l2cache(void) {}
#endif

extern struct smp_operations imx_smp_ops;

#endif
