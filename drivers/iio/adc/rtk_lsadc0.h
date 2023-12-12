/* SPDX-License-Identifier: GPL-2.0 OR MIT */
/*
 *
 * Realtek Low Speed ADC driver
 *
 * Copyright (C) 2020 Realtek Semiconductor Corporation.
 */

#ifndef __RTK_LSADC0_H_
#define __RTK_LSADC0_H_

/* LSADC0 registers */
#define LSADC0_PAD0_ADDR		(0x00)
#define LSADC0_PAD1_ADDR		(0x04)

#define LSADC0_CTRL_ADDR		(0x20)
#define LSADC0_STATUS_ADDR		(0x24)
#define LSADC0_ANALOG_CTRL_ADDR		(0x28)

#define LSADC0_PAD_LEVEL_SET_NUMBER	(6)
#define LSADC0_PAD0_LEVEL_SET0_ADDR	(0x30)
#define LSADC0_PAD0_LEVEL_SET1_ADDR	(0x34)
#define LSADC0_PAD0_LEVEL_SET2_ADDR	(0x38)
#define LSADC0_PAD0_LEVEL_SET3_ADDR	(0x3c)
#define LSADC0_PAD0_LEVEL_SET4_ADDR	(0x40)
#define LSADC0_PAD0_LEVEL_SET5_ADDR	(0x44)

#define LSADC0_PAD1_LEVEL_SET0_ADDR	(0x48)
#define LSADC0_PAD1_LEVEL_SET1_ADDR	(0x4c)
#define LSADC0_PAD1_LEVEL_SET2_ADDR	(0x50)
#define LSADC0_PAD1_LEVEL_SET3_ADDR	(0x54)
#define LSADC0_PAD1_LEVEL_SET4_ADDR	(0x58)
#define LSADC0_PAD1_LEVEL_SET5_ADDR	(0x5c)

#define LSADC0_POWER_ADDR		(0x80)

/* LSADC0_PAD0_ADDR - bit fields */
/* LSADC0_PAD1_ADDR - bit fields */
#define LSADC0_PAD_ACTIVE_MASK		BIT(31)
#define LSADC0_PAD_THRED_SHIFT		(16)
#define LSADC0_PAD_THRED_MASK		GENMASK(23, 16)
#define LSADC0_PAD_SW_MASK		BIT(12)
#define LSADC0_PAD_CTRL_MASK		BIT(8)
#define LSADC0_PAD_ADC_VAL_MASK		GENMASK(7, 0)

/* LSADC0_CTRL_ADDR - bit fields */
#define LSADC0_CTRL_SEL_WAIT_MASK	GENMASK(31, 28)
#define LSADC0_CTRL_SEL_WAIT_DEFAULT	BIT(31)
#define LSADC0_CTRL_DEBOUNCE_SHIFT	(20)
#define LSADC0_CTRL_DEBOUNCE_MASK	GENMASK(23, 20)
#define LSADC0_CTRL_ENABLE_MASK		BIT(0)

/* LSADC0_STATUS_ADDR - bit fields */
#define LSADC0_STATUS_IRQ_EN_MASK	GENMASK(25, 24)
#define LSADC0_STATUS_PAD_CNT_MASK	BIT(20)
#define LSADC0_STATUS_ADC_BUSY_MASK	BIT(19)
#define LSADC0_STATUS_PAD_CTRL_MASK	GENMASK(16, 12)
#define LSADC0_STATUS_PAD1_STATUS_MASK	BIT(1)
#define LSADC0_STATUS_PAD0_STATUS_MASK	BIT(0)
#define LSADC0_STATUS_ENABLE_IRQ	LSADC0_STATUS_IRQ_EN_MASK

/* LSADC0_ANALOG_CTRL_ADDR - bit fields */
#define LSADC0_ANALOG_CTRL_JD_PWR_MASK	BIT(0)
#define LSADC0_ANALOG_CTRL_VALUE	LSADC0_ANALOG_CTRL_JD_PWR_MASK

/* LSADC0_PAD1_LEVEL_SET0_ADDR - bit fields */
#define LSADC0_LEVEL_TOP_BOUND_SHIFT	(24)
#define LSADC0_LEVEL_TOP_BOUND_MASK	GENMASK(31, 24)
#define LSADC0_LEVEL_LOW_BOUND_SHIFT	(16)
#define LSADC0_LEVEL_LOW_BOUND_MASK	GENMASK(23, 16)
#define LSADC0_LEVEL_BLK_EN_SHIFT	(15)
#define LSADC0_LEVEL_BLK_EN_MASK	BIT(15)
#define LSADC0_LEVEL_INT_EN_SHIFT	(1)
#define LSADC0_LEVEL_INT_EN_MASK	BIT(1)
#define LSADC0_LEVEL_INT_PEND_MASK	BIT(0)
#define LSADC0_LEVEL_ENABLE_BLOCK	LSADC0_LEVEL_BLK_EN_MASK
#define LSADC0_LEVEL_ENABLE_IRQ		LSADC0_LEVEL_INT_EN_MASK
#define LSADC0_LEVEL_CLEAR_IRQ		LSADC0_LEVEL_INT_PEND_MASK

/* LSADC0_POWER_ADDR - bit fields */
#define LSADC0_CLK_GATING_EN_MASK	BIT(0)
#define LSADC0_CLK_GATING_EN		LSADC0_CLK_GATING_EN_MASK

/* CRT LSADC_PG_ADDR - bit fields */
#define CRT_LSADC_VDDMUX_SEL_SHIFT	(10)
#define CRT_LSADC_VDDMUX_SEL_MASK	BIT(10)
#define CRT_LSADC_VDD_MUX1_SHIFT	(6)
#define CRT_LSADC_VDD_MUX1_MASK		GENMASK(9, 6)
#define CRT_LSADC_VDD_MUX2_SHIFT	(2)
#define CRT_LSADC_VDD_MUX2_MASK		GENMASK(5, 2)
#define CRT_LSADC_VDDMUX_EN_SHIFT	(1)
#define CRT_LSADC_VDDMUX_EN_MASK	BIT(1)
#define CRT_LSADC_PG_VALUE		CRT_LSADC_VDDMUX_EN_MASK

#define MICROVOLT_HIGH			(3300000)

#endif /*__RTK_LSADC0_H_ */

