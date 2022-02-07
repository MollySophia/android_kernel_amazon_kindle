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
#ifndef __BD7181X_EVENTS_H__
#define __BD7181X_EVENTS_H__

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

#define ENCODE_EVENT(x,y) ((x << 8) | (y))
#define DECODE_EVENT(x, y, z) (y) = (((x) >> 8) & 0xFF); (z) = ((x) & 0xFF)

/*!
 * Events(except evt recorder evts) are numbered as a combination of IRQ# and Bit positon of 
 * the event in the interrupt register. MAX77696 has matched bit positions for same event 
 * in all of the int, mask and status regs, so we should be good here
 */
/* TOPSYS Events has IRQs for each bit */
#define EVENT_TOPS_BUCK_AST 		ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(7))
#define EVENT_TOPS_DCIN_AST			ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(6)) 
#define EVENT_TOPS_VSYS_AST 		ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(5)) 
#define EVENT_TOPS_CHG_AST 			ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(4)) 
#define EVENT_TOPS_BAT_AST 			ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(3)) 
#define EVENT_TOPS_BMON_AST  		ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(2)) 
#define EVENT_TOPS_TMP_AST 			ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(1)) 
#define EVENT_TOPS_ALM_AST			ENCODE_EVENT(BD7181X_REG_INT_STAT, BIT(0)) 

/* BUCK events
*
*/
#define EVENT_BUCK_LED_SCP			ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(7)) 
#define EVENT_BUCK_LED_OCP			ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(6)) 
#define EVENT_BUCK_LED_OVP			ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(5)) 
#define EVENT_BUCK_BUCK5FAULT		ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(4)) 
#define EVENT_BUCK_BUCK4FAULT		ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(3)) 
#define EVENT_BUCK_BUCK3FAULT		ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(2)) 
#define EVENT_BUCK_BUCK2FAULT		ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(1)) 
#define EVENT_BUCK_BUCK1FAULT		ENCODE_EVENT(BD7181X_REG_INT_STAT_01, BIT(0)) 

/* DCIN1 events
*
*/
#define EVENT_DCIN_OV_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_02, BIT(5)) 
#define EVENT_DCIN_OV_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_02, BIT(4)) 
#define EVENT_DCIN_CLPS_OUT			ENCODE_EVENT(BD7181X_REG_INT_STAT_02, BIT(3)) 
#define EVENT_DCIN_CLPS_IN			ENCODE_EVENT(BD7181X_REG_INT_STAT_02, BIT(2)) 
#define EVENT_DCIN_DCIN_RMV			ENCODE_EVENT(BD7181X_REG_INT_STAT_02, BIT(1)) 

/* DCIN2 events
*
*/
#define EVENT_DCIN_WDOGB			ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(6)) 
#define EVENT_DCIN_PWRON_PRESS		ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(5)) 
#define EVENT_DCIN_PWRON_SHORT		ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(4)) 
#define EVENT_DCIN_PWRON_MID		ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(3)) 
#define EVENT_DCIN_PWRON_LONG		ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(2)) 
#define EVENT_DCIN_MON_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(1)) 
#define EVENT_DCIN_MON_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_03, BIT(0)) 

/* VSYS events
*
*/
#define EVENT_VSYS_MON_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_04, BIT(7))
#define EVENT_VSYS_MON_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_04, BIT(6))
#define EVENT_VSYS_LO_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_04, BIT(3))
#define EVENT_VSYS_LO_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_04, BIT(2))
#define EVENT_VSYS_UV_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_04, BIT(1))
#define EVENT_VSYS_UV_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_04, BIT(0))

/* CHGS events
*
*/
#define EVENT_CHG_CHG_TRNS			ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(7))
#define EVENT_CHG_TMP_CRNS			ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(6))
#define EVENT_CHG_BAT_MNT_IN		ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(5))
#define EVENT_CHG_BAT_MNY_OUT		ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(4))
#define EVENT_CHG_WDT_EXP			ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(3))
#define EVENT_CHG_EXTEMP_TOUTS		ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(2))
#define EVENT_CHG_ROHM_FACTORY		ENCODE_EVENT(BD7181X_REG_INT_STAT_05, BIT(0))

/* BAT events
*
*/
#define EVENT_BAT_TH_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_06, BIT(7))
#define EVENT_BAT_TH_RMV			ENCODE_EVENT(BD7181X_REG_INT_STAT_06, BIT(6))
#define EVENT_BAT_BAT_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_06, BIT(5))
#define EVENT_BAT_BAT_RMV			ENCODE_EVENT(BD7181X_REG_INT_STAT_06, BIT(4))
#define EVENT_BAT_TMP_OUT_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_06, BIT(1))
#define EVENT_BAT_TMP_OUR_RES		ENCODE_EVENT(BD7181X_REG_INT_STAT_06, BIT(0))

/* BAT MON
*
*/
#define EVENT_VBAT_OV_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(7))
#define EVENT_VBAT_OV_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(6))
#define EVENT_VBAT_LO_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(5))
#define EVENT_VBAT_LO_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(4))
#define EVENT_VBAT_SHT_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(3))
#define EVENT_VBAT_SHT_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(2))
#define EVENT_VBAT_DBAT_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_07, BIT(1))

#define EVENT_VBAT_MON_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_08, BIT(1))
#define EVENT_VBAT_MON_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_08, BIT(0))

#define EVENT_VBAT_CC_MON3_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_09, BIT(2))
#define EVENT_VBAT_CC_MON2_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_09, BIT(1))
#define EVENT_VBAT_CC_MON1_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_09, BIT(0))

#define EVENT_VBAT_OCUR3_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_10, BIT(5))
#define EVENT_VBAT_OCUR3_RES		ENCODE_EVENT(BD7181X_REG_INT_STAT_10, BIT(4))
#define EVENT_VBAT_OCUR2_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_10, BIT(3))
#define EVENT_VBAT_OCUR2_RES		ENCODE_EVENT(BD7181X_REG_INT_STAT_10, BIT(2))
#define EVENT_VBAT_OCUR1_DET		ENCODE_EVENT(BD7181X_REG_INT_STAT_10, BIT(1))
#define EVENT_VBAT_OCUR1_RES		ENCODE_EVENT(BD7181X_REG_INT_STAT_10, BIT(0))

/* TMP evetns
*
*/
#define EVENT_TMP_VF_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(7))
#define EVENT_TMP_VF_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(6))
#define EVENT_TMP_VF125_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(5))
#define EVENT_TMP_VF125_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(4))
#define EVENT_TMP_OVTMP_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(3))
#define EVENT_TMP_OVTMP_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(2))
#define EVENT_TMP_LOTMP_DET			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(1))
#define EVENT_TMP_LOTMP_RES			ENCODE_EVENT(BD7181X_REG_INT_STAT_11, BIT(0))

/* ALARM events
*
*/
#define EVENT_ALM_ALM2				ENCODE_EVENT(BD7181X_REG_INT_STAT_12, BIT(2))
#define EVENT_ALM_ALM1				ENCODE_EVENT(BD7181X_REG_INT_STAT_12, BIT(1))
#define EVENT_ALM_ALM0				ENCODE_EVENT(BD7181X_REG_INT_STAT_12, BIT(0))

#endif	/* __BD7181X_EVENTS_H__ */