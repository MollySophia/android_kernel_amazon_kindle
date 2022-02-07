/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc. All Rights Reserved.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef _MXCFB_EPDC_KERNEL
#define _MXCFB_EPDC_KERNEL

#define FLAG_SCAN_X_INVERT  1
struct imx_epdc_fb_mode {
    struct fb_videomode *vmode;
    int vscan_holdoff;
    int sdoed_width;
    int sdoed_delay;
    int sdoez_width;
    int sdoez_delay;
    int gdclk_hp_offs;
    int gdsp_offs;
    int gdoe_offs;
    int gdclk_offs;
    int num_ce;
    int physical_width; // Physical width in mm
    int physical_height; // Physical height in mm
    int material; // Display material
};

struct imx_epdc_fb_platform_data {
    struct imx_epdc_fb_mode *epdc_mode;
    int num_modes;
    int (*get_pins) (void);
    void (*put_pins) (void);
    void (*enable_pins) (void);
    void (*disable_pins) (void);
};

void mxc_epdc_fb_set_waveform_modes(struct mxcfb_waveform_modes *modes,
						struct fb_info *info);
int mxc_epdc_fb_set_temperature(int temperature, struct fb_info *info);
int mxc_epdc_fb_set_auto_update(u32 auto_mode, struct fb_info *info);
int mxc_epdc_fb_send_update(struct mxcfb_update_data *upd_data,
				   struct fb_info *info);
/*int mxc_epdc_fb_wait_update_complete(
				struct mxcfb_update_marker_data *marker_data,
				struct fb_info *info); */
int mxc_epdc_fb_set_pwrdown_delay(u32 pwrdown_delay,
					    struct fb_info *info);
int mxc_epdc_get_pwrdown_delay(struct fb_info *info);
int mxc_epdc_fb_set_upd_scheme(u32 upd_scheme, struct fb_info *info);

void mxc_spdc_fb_set_waveform_modes(struct mxcfb_waveform_modes *modes,
						struct fb_info *info);
int mxc_spdc_fb_set_temperature(int temperature, struct fb_info *info);
int mxc_spdc_fb_set_auto_update(u32 auto_mode, struct fb_info *info);
int mxc_spdc_fb_send_update(struct mxcfb_update_data *upd_data,
				   struct fb_info *info);
int mxc_spdc_fb_wait_update_complete(
				struct mxcfb_update_marker_data *marker_data,
				struct fb_info *info);
int mxc_spdc_fb_set_pwrdown_delay(u32 pwrdown_delay,
					    struct fb_info *info);
int mxc_spdc_get_pwrdown_delay(struct fb_info *info);
int mxc_spdc_fb_set_upd_scheme(u32 upd_scheme, struct fb_info *info);
#endif
