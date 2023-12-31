// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Realtek Semiconductor Corporation
 * Author: Cheng-Yu Lee <cylee12@realtek.com>
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include "common.h"
#include "clk-pll.h"
#include "clk-regmap-gate.h"
#include "clk-regmap-mux.h"
#include "clk-det.h"
#include "reset.h"
#include <dt-bindings/clock/rtd1619b-clk.h>
#include <dt-bindings/reset/rtd1619b-reset.h>
#include "of-conf.h"

#define DIV_DV(_r, _d, _v)    { .rate = _r, .div = _d, .val = _v, }
#define FREQ_NF_MASK          (0x7FFFF)
#define FREQ_NF(_r, _n, _f)   { .rate = _r, .val = ((_n) << 11) | (_f), }
#define FREQ_MNO_MASK         (0xe3FF0)
#define FREQ_MNO(_r, _m, _n, _o) \
	{ .rate = _r, .val = ((_m) << 4) | ((_n) << 12) | ((_o) << 17), }

static const char * const default_parent[] = { "osc27m" };

static const struct div_table scpu_div_tbl[] = {
	DIV_DV(1000000000,  1, 0x00),
	DIV_DV(500000000,   2, 0x88),
	DIV_DV(250000000,   4, 0x90),
	DIV_DV(200000000,   8, 0xA0),
	DIV_DV(100000000,  10, 0xA8),
	DIV_TABLE_END
};

static const struct freq_table scpu_tbl[] = {
	FREQ_NF(1000000000, 34,   75),
	FREQ_NF(1100000000, 37, 1517),
	FREQ_NF(1200000000, 41,  910),
	FREQ_NF(1200000000, 41,    0),
	FREQ_NF(1300000000, 45,  303),
	FREQ_NF(1400000000, 48, 1745),
	FREQ_NF(1500000000, 52, 1137),
	FREQ_NF(1600000000, 56,  530),
	FREQ_NF(1700000000, 60,    0),
	FREQ_NF(1800000000, 63, 1365),
	FREQ_NF(1900000000, 67,  758),
	FREQ_NF(2000000000, 71,  151),
	FREQ_TABLE_END
};

static struct clk_pll_div pll_scpu = {
	.div_ofs = 0x108,
	.div_shift  = 8,
	.div_width  = 8,
	.div_tbl    = scpu_div_tbl,
	.clkp       = {
		.ssc_ofs   = 0x500,
		.pll_ofs   = CLK_OFS_INVALID,
		.pll_type  = CLK_PLL_TYPE_NF_SSC,
		.freq_tbl  = scpu_tbl,
		.freq_mask = FREQ_NF_MASK,
		.clkr.hw.init = &(struct clk_init_data) {
			.name         = "pll_scpu",
			.ops          = &clk_pll_div_ops,
			.parent_names = default_parent,
			.num_parents  = 1,
			.flags        = CLK_IGNORE_UNUSED |
					CLK_GET_RATE_NOCACHE,
		},
	},
};

static const struct div_table bus_div_tbl[] = {
	DIV_DV(257000000, 1, 0),
	DIV_DV(129000000, 2, 2),
	DIV_DV(65000000,  4, 3),
	DIV_TABLE_END
};

static const struct freq_table bus_tbl[] = {
	FREQ_NF(459000000, 0x1f, 0x000),
	FREQ_NF(486000000, 0x21, 0x000),
	FREQ_NF(499500000, 0x22, 0x000),
	FREQ_NF(594000000, 0x23, 0x000),
	FREQ_TABLE_END
};

static struct clk_pll_div pll_bus = {
	.div_ofs = 0x030,
	.div_shift  = 0,
	.div_width  = 2,
	.div_tbl    = bus_div_tbl,
	.clkp       = {
		.ssc_ofs   = 0x520,
		.pll_ofs   = CLK_OFS_INVALID,
		.pll_type  = CLK_PLL_TYPE_NF_SSC,
		.freq_tbl  = bus_tbl,
		.freq_mask = FREQ_NF_MASK,
		.clkr.hw.init = &(struct clk_init_data) {
			.name         = "pll_bus",
			.ops          = &clk_pll_div_ops,
			.parent_names = default_parent,
			.num_parents  = 1,
			.flags        = CLK_IGNORE_UNUSED |
					CLK_GET_RATE_NOCACHE,
		},
		.clkr.shared = 1,
	},
};

static const struct div_table dcsb_div_tbl[] = {
	DIV_DV(300000000, 1, 0),
	DIV_DV(1, 2, 2),
	DIV_DV(1, 4, 3),
	DIV_TABLE_END
};

static const struct freq_table dcsb_tbl[] = {
	FREQ_NF(351000000, 0x1a, 0x000),
	FREQ_NF(405000000, 0x1b, 0x000),
	FREQ_NF(459000000, 0x1f, 0x000),
	FREQ_NF(472000000, 0x1f, 0x7b4),
	FREQ_NF(499500000, 0x22, 0x000),
	FREQ_NF(567000000, 0x27, 0x000),
	FREQ_NF(594000000, 0x29, 0x000),
	FREQ_TABLE_END
};

static struct clk_pll_div pll_dcsb = {
	.div_ofs = 0x030,
	.div_shift  = 2,
	.div_width  = 2,
	.div_tbl    = dcsb_div_tbl,
	.clkp       = {
		.ssc_ofs   = 0x540,
		.pll_ofs   = CLK_OFS_INVALID,
		.pll_type  = CLK_PLL_TYPE_NF_SSC,
		.freq_tbl  = dcsb_tbl,
		.freq_mask = FREQ_NF_MASK,
		.clkr.hw.init = &(struct clk_init_data) {
			.name         = "pll_dcsb",
			.ops          = &clk_pll_div_ops,
			.parent_names = default_parent,
			.num_parents  = 1,
			.flags        = CLK_IGNORE_UNUSED |
					CLK_GET_RATE_NOCACHE,
		},
		.clkr.shared = 1,
	},
};

static struct clk_fixed_factor clk_sys = {
	.div     = 1,
	.mult    = 1,
	.hw.init = &(struct clk_init_data) {
		.name         = "clk_sys",
		.ops          = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "pll_bus" },
		.num_parents  = 1,
		.flags        = CLK_SET_RATE_PARENT,
	},
};

static struct clk_fixed_factor clk_sysh = {
	.div     = 1,
	.mult    = 1,
	.hw.init = &(struct clk_init_data) {
		.name         = "clk_sysh",
		.ops          = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "pll_dcsb" },
		.num_parents  = 1,
		.flags        = CLK_SET_RATE_PARENT,
	},
};

static const struct freq_table ddsx_tbl[] = {
	FREQ_NF(432000000, 29, 0),
	FREQ_TABLE_END
};

static struct clk_pll pll_ddsa = {
	.ssc_ofs   = 0x560,
	.pll_ofs   = 0x120,
	.pow_loc   = CLK_PLL_CONF_POW_LOC_CTL3,
	.pll_type  = CLK_PLL_TYPE_NF_SSC,
	.freq_tbl  = ddsx_tbl,
	.freq_mask = FREQ_NF_MASK,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_ddsa",
		.ops          = &clk_pll_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE,
	},
};

static const struct freq_table gpu_tbl[] = {
	FREQ_NF(400000000, 0x1a, 0x509),
	FREQ_NF(450000000, 0x1e, 0x2aa),
	FREQ_NF(500000000, 0x22, 0x04b),
	FREQ_NF(550000000, 0x25, 0x5ed),
	FREQ_NF(650000000, 0x2d, 0x12f),
	FREQ_NF(702000000, 0x31, 0x000),
	FREQ_NF(729000000, 0x33, 0x000),
	FREQ_NF(769000000, 0x36, 0x000),
	FREQ_NF(810000000, 0x39, 0x000),
	FREQ_NF(837000000, 0x3b, 0x000),
	FREQ_NF(850500000, 0x3c, 0x000),
	FREQ_TABLE_END
};

static struct clk_pll pll_gpu = {
	.ssc_ofs   = 0x5A0,
	.pll_ofs   = 0x1C0,
	.pow_loc   = CLK_PLL_CONF_POW_LOC_CTL2,
	.pll_type  = CLK_PLL_TYPE_NF_SSC,
	.freq_tbl  = gpu_tbl,
	.freq_mask = FREQ_NF_MASK,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_gpu",
		.ops          = &clk_pll_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_GET_RATE_NOCACHE,
	},
	CLK_PLL_CONF_RS(0x0003c000, 0x00020000),
	CLK_PLL_CONF_PI_BPS(),
};

static const struct freq_table ve_tbl[] = {
	FREQ_MNO(432000000, 0x0d, 0, 0),
	FREQ_MNO(540000000, 0x11, 0, 0),
	FREQ_MNO(553000000, 0x26, 1, 0),
	FREQ_MNO(621000000, 0x14, 0, 0),
	FREQ_MNO(621000000, 0x2b, 1, 0),
	FREQ_MNO(648000000, 0x2d, 1, 0),
	FREQ_MNO(675000000, 0x2f, 1, 0),
	FREQ_TABLE_END
};

static struct clk_pll pll_ve1 = {
	.ssc_ofs   = CLK_OFS_INVALID,
	.pll_ofs   = 0x114,
	.pow_loc   = CLK_PLL_CONF_POW_LOC_CTL2,
	.pll_type  = CLK_PLL_TYPE_MNO_V2,
	.freq_tbl  = ve_tbl,
	.freq_mask = FREQ_MNO_MASK,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_ve1",
		.ops          = &clk_pll_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_GATE,
	},
	CLK_PLL_CONF_RS(0x0081c000, 0x00800000),
};

static struct clk_pll pll_ve2 = {
	.ssc_ofs   = CLK_OFS_INVALID,
	.pll_ofs   = 0x1D0,
	.pow_loc   = CLK_PLL_CONF_POW_LOC_CTL2,
	.pll_type  = CLK_PLL_TYPE_MNO_V2,
	.freq_tbl  = ve_tbl,
	.freq_mask = FREQ_MNO_MASK,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_ve2",
		.ops          = &clk_pll_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED | CLK_SET_RATE_GATE,
	},
	CLK_PLL_CONF_RS(0x0081c000, 0x00800000),
};

static struct clk_pll_dif pll_dif = {
	.ssc_ofs   = 0x634,
	.pll_ofs   = 0x624,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_dif",
		.ops          = &clk_pll_dif_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_GET_RATE_NOCACHE,
	},
	.adtv_conf = {
		0x02000949, 0x00030c00, 0x204004ca, 0x400C6004,
		0x00431c00, 0x00431c03, 0x02000979, 0x004884ca,
	},
};

static struct clk_pll_psaud pll_psaud1a = {
	.reg = 0x130,
	.id  = CLK_PLL_PSAUD1A,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_psaud1a",
		.ops          = &clk_pll_psaud_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_IGNORE_UNUSED | CLK_SET_RATE_UNGATE,
	},
};

static struct clk_pll_psaud pll_psaud2a = {
	.reg = 0x130,
	.id  = CLK_PLL_PSAUD2A,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_psaud2a",
		.ops          = &clk_pll_psaud_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_IGNORE_UNUSED | CLK_SET_RATE_UNGATE,
	},
};

static const struct freq_table npu_tbl[] = {
	FREQ_NF(600000000, 0x29, 0x38e),
	FREQ_NF(700000000, 0x30, 0x6d0),
	FREQ_NF(729000000, 0x33, 0x000),
	FREQ_NF(756000000, 0x35, 0x000),
	FREQ_NF(850500000, 0x3c, 0x000),
	FREQ_TABLE_END
};

static struct clk_pll pll_npu = {
	.ssc_ofs   = 0x6b0,
	.pll_ofs   = 0x1c8,
	.pll_type  = CLK_PLL_TYPE_NF_SSC,
	.freq_tbl  = npu_tbl,
	.freq_mask = FREQ_NF_MASK,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_npu",
		.ops          = &clk_pll_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_GET_RATE_NOCACHE,
	},
	CLK_PLL_CONF_RS(0x0003c000, 0x00020000),
	CLK_PLL_CONF_PI_BPS(),
};

static const struct freq_table hifi_tbl[] = {
	FREQ_NF(405000000, 0x1b, 0x000),
	FREQ_NF(486000000, 0x21, 0x000),
	FREQ_NF(499500000, 0x22, 0x000),
	FREQ_NF(702000000, 0x31, 0x000),
	FREQ_NF(796500000, 0x38, 0x000),
	FREQ_NF(810000000, 0x39, 0x000),
	FREQ_TABLE_END
};

static struct clk_pll pll_hifi = {
	.ssc_ofs   = 0x6e0,
	.pll_ofs   = 0x1d8,
	.pll_type  = CLK_PLL_TYPE_NF_SSC,
	.freq_tbl  = hifi_tbl,
	.freq_mask = FREQ_NF_MASK,
	.clkr.hw.init = &(struct clk_init_data) {
		.name         = "pll_hifi",
		.ops          = &clk_pll_ops,
		.parent_names = default_parent,
		.num_parents  = 1,
		.flags        = CLK_GET_RATE_NOCACHE,
	},
	CLK_PLL_CONF_RS(0x0003c000, 0x00020000),
	CLK_PLL_CONF_PI_BPS(),
};

static struct clk_det clk_det_pll_bus = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_bus", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x424,
};

static struct clk_det clk_det_pll_dcsb = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_dcsb", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x428,
};

static struct clk_det clk_det_pll_acpu = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_acpu", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x42c,
};

static struct clk_det clk_det_pll_ddsa = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_ddsa", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x430,
};

static struct clk_det clk_det_pll_gpu = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_gpu", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x438,
};

static struct clk_det clk_det_pll_ve1 = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_ve1", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x43c,
};

static struct clk_det clk_det_pll_ve2 = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_ve2", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x440,
};

static struct clk_det clk_det_pll_npu = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_npu", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x444,
};

static struct clk_det clk_det_pll_hifi = {
	.clkr.hw.init = CLK_HW_INIT_NO_PARENT("ref_pll_hifi", &clk_det_ops, CLK_GET_RATE_NOCACHE),
	.type = CLK_DET_TYPE_CRT,
	.ofs  = 0x448,
};

static struct clk_hw *cc_hws[] = {
	[RTD1619B_CRT_PLL_SCPU]    = &__clk_pll_div_hw(&pll_scpu),
	[RTD1619B_CRT_PLL_BUS]     = &__clk_pll_div_hw(&pll_bus),
	[RTD1619B_CRT_PLL_DCSB]    = &__clk_pll_div_hw(&pll_dcsb),
	[RTD1619B_CRT_CLK_SYS]     = &clk_sys.hw,
	[RTD1619B_CRT_CLK_SYSH]    = &clk_sysh.hw,
	[RTD1619B_CRT_PLL_DDSA]    = &__clk_pll_hw(&pll_ddsa),
	[RTD1619B_CRT_PLL_GPU]     = &__clk_pll_hw(&pll_gpu),
	[RTD1619B_CRT_PLL_VE1]     = &__clk_pll_hw(&pll_ve1),
	[RTD1619B_CRT_PLL_VE2]     = &__clk_pll_hw(&pll_ve2),
	[RTD1619B_CRT_PLL_DIF]     = &__clk_pll_dif_hw(&pll_dif),
	[RTD1619B_CRT_PLL_PSAUD1A] = &__clk_pll_psaud_hw(&pll_psaud1a),
	[RTD1619B_CRT_PLL_PSAUD2A] = &__clk_pll_psaud_hw(&pll_psaud2a),
	[RTD1619B_CRT_PLL_NPU]     = &__clk_pll_hw(&pll_npu),
	[RTD1619B_CRT_PLL_HIFI]    = &__clk_pll_hw(&pll_hifi),

	[RTD1619B_CRT_CLK_DET_PLL_BUS]  = &__clk_det_hw(&clk_det_pll_bus),
	[RTD1619B_CRT_CLK_DET_PLL_DCSB] = &__clk_det_hw(&clk_det_pll_dcsb),
	[RTD1619B_CRT_CLK_DET_PLL_ACPU] = &__clk_det_hw(&clk_det_pll_acpu),
	[RTD1619B_CRT_CLK_DET_PLL_DDSA] = &__clk_det_hw(&clk_det_pll_ddsa),
	[RTD1619B_CRT_CLK_DET_PLL_GPU]  = &__clk_det_hw(&clk_det_pll_gpu),
	[RTD1619B_CRT_CLK_DET_PLL_VE1]  = &__clk_det_hw(&clk_det_pll_ve1),
	[RTD1619B_CRT_CLK_DET_PLL_VE2]  = &__clk_det_hw(&clk_det_pll_ve2),
	[RTD1619B_CRT_CLK_DET_PLL_NPU]  = &__clk_det_hw(&clk_det_pll_npu),
	[RTD1619B_CRT_CLK_DET_PLL_HIFI] = &__clk_det_hw(&clk_det_pll_hifi),
};

static const char * const ve_parents[] = {
	"",
	"clk_sysh",
	"pll_ve1",
	"pll_ve2",
};

static struct clk_composite_data cc_composites[] = {
	{
		.id            = RTD1619B_CRT_CLK_GPU,
		.mux_ofs       = CLK_OFS_INVALID,
		.gate_ofs      = 0x050,
		.gate_shift    = 18,
		.gate_write_en = 1,
		.parent_names  = (const char *[]){ "pll_gpu" },
		.num_parents   = 1,
		.name          = "clk_gpu",
		.flags         = CLK_SET_RATE_PARENT,
	},
	{
		.id            = RTD1619B_CRT_CLK_VE1,
		.gate_ofs      = 0x050,
		.gate_shift    = 20,
		.gate_write_en = 1,
		.mux_ofs       = 0x04C,
		.mux_width     = 3,
		.mux_shift     = 0,
		.parent_names  = ve_parents,
		.num_parents   = ARRAY_SIZE(ve_parents),
		.name          = "clk_ve1",
		.flags         = CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT,
	},
	{
		.id            = RTD1619B_CRT_CLK_VE2,
		.gate_ofs      = 0x050,
		.gate_shift    = 22,
		.gate_write_en = 1,
		.mux_ofs       = 0x04C,
		.mux_width     = 3,
		.mux_shift     = 3,
		.parent_names  = ve_parents,
		.num_parents   = ARRAY_SIZE(ve_parents),
		.name          = "clk_ve2",
		.flags         = CLK_SET_RATE_PARENT |
				 CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED,
	},
	{
		.id            = RTD1619B_CRT_CLK_VE3,
		.gate_ofs      = 0x05C,
		.gate_shift    = 26,
		.gate_write_en = 1,
		.mux_ofs       = 0x04C,
		.mux_width     = 3,
		.mux_shift     = 6,
		.parent_names  = ve_parents,
		.num_parents   = ARRAY_SIZE(ve_parents),
		.name          = "clk_ve3",
		.flags         = CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT,
	},
	{
		.id            = RTD1619B_CRT_CLK_VE3_BPU,
		.gate_ofs      = CLK_OFS_INVALID,
		.mux_ofs       = 0x04C,
		.mux_width     = 3,
		.mux_shift     = 9,
		.parent_names  = ve_parents,
		.num_parents   = ARRAY_SIZE(ve_parents),
		.name          = "clk_ve3_bpu",
		.flags         = CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT,
	},
};


#define GATE_COMMON(_id, _name, _parent, _flags, _ofs, _shift) \
	CLK_GATE_DATA(_id, _name, _parent, _flags, _ofs, _shift, 1, 0)

#define GATE_NORMAL(_id, _name, _parent, _ofs, _shift) \
	GATE_COMMON(_id, _name, _parent, 0, _ofs, _shift)

#define GATE_IGNORED(_id, _name, _parent, _ofs, _shift) \
	GATE_COMMON(_id, _name, _parent, CLK_IGNORE_UNUSED, _ofs, _shift)

#define GATE_CRITICAL(_id, _name, _parent, _ofs, _shift) \
	GATE_COMMON(_id, _name, _parent, CLK_IS_CRITICAL, _ofs, _shift)

static struct clk_gate_data cc_gates[] = {
	GATE_CRITICAL(RTD1619B_CRT_CLK_EN_MISC, "misc", NULL, 0x50, 0),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_PCIE0, "pcie0", NULL, 0x50, 2),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_GSPI, "gspi", "misc", 0x50, 6),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SDS, "sds", NULL, 0x50, 12),
	GATE_IGNORED(RTD1619B_CRT_CLK_EN_HDMI, "hdmi", NULL, 0x50, 14),
	GATE_IGNORED(RTD1619B_CRT_CLK_EN_CP, "cp", NULL, 0x54, 2),
	GATE_IGNORED(RTD1619B_CRT_CLK_EN_TP, "tp", NULL, 0x54, 6),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_NF, "nf", NULL, 0x54, 10),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_EMMC, "emmc", NULL, 0x54, 12),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SD, "sd", NULL, 0x54, 14),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SDIO_IP, "sdio_ip", NULL, 0x54, 16),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MIPI, "mipi", NULL, 0x54, 18),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_EMMC_IP, "emmc_ip", NULL, 0x54, 20),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SDIO, "sdio", NULL, 0x54, 22),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SD_IP, "sd_ip", NULL, 0x54, 24),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_CABLERX, "cablerx", NULL, 0x54, 26),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_TPB, "tpb", NULL, 0x54, 28),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MISC_SC1, "misc_sc1", "misc", 0x54, 30),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MISC_I2C_3, "misc_i2c_3", "misc", 0x58, 0),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_JPEG, "jpeg", NULL, 0x58, 4),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MISC_SC0, "misc_sc0", "misc", 0x58, 10),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_HDMIRX, "hdmirx", NULL, 0x58, 26),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_HSE, "hse", NULL, 0x58, 28),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_UR2, "ur2", "misc", 0x58, 30),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_UR1, "ur1", "misc", 0x5c, 0),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_FAN, "fan", "misc", 0x5c, 2),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SATA_WRAP_SYS, "sata_wrap_sys", NULL, 0x5c, 8),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SATA_WRAP_SYSH, "sata_wrap_sysh", NULL, 0x5c, 10),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_SATA_MAC_SYSH, "sata_mac_sysh", NULL, 0x5c, 12),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_R2RDSC, "r2rdsc", NULL, 0x5c, 14),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_TPC, "tpc", NULL, 0x5c, 16),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_PCIE1, "pcie1", NULL, 0x5c, 18),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MISC_I2C_4, "misc_i2c_4", "misc", 0x5c, 20),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MISC_I2C_5, "misc_i2c_5", "misc", 0x5c, 22),
	GATE_IGNORED(RTD1619B_CRT_CLK_EN_TSIO, "tsio", NULL, 0x5c, 24),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_EDP, "edp", NULL, 0x5c, 28),
	GATE_IGNORED(RTD1619B_CRT_CLK_EN_TSIO_TRX, "tsio_trx", NULL, 0x5c, 30),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_PCIE2, "pcie2", NULL, 0x8c, 0),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_LITE, "lite", NULL, 0x8c, 6),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_MIPI_DSI, "mipi_dsi", NULL, 0x8c, 8),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_NPUPP, "npupp", NULL, 0x8c, 10),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_NPU, "npu", NULL, 0x8c, 12),
	GATE_NORMAL(RTD1619B_CRT_CLK_EN_HDMITOP, "hdmitop", NULL, 0x8c, 20),
};

static struct rtk_reset_bank cc_reset_banks[] = {
	{ .ofs = 0x000, .write_en = 1, },
	{ .ofs = 0x004, .write_en = 1, },
	{ .ofs = 0x008, .write_en = 1, },
	{ .ofs = 0x00C, .write_en = 1, },
	{ .ofs = 0x068, .write_en = 1, },
	{ .ofs = 0x090, .write_en = 1, },
	{ .ofs = 0x454, },
	{ .ofs = 0x458, },
	{ .ofs = 0x464, },
};

static struct rtk_reset_initdata cc_reset_initdata = {
	.banks     = cc_reset_banks,
	.num_banks = ARRAY_SIZE(cc_reset_banks),
};

static struct clk_hw_map rtd1619b_sys_map = {
	.group = &(struct clk_hw_group) {
		.hws = cc_hws,
		.num_hws = ARRAY_SIZE(cc_hws),
	},
	.start_index = 0,
};

static struct clk_hw_map rtd1619b_sys_n_map = {
	.group = &(struct clk_hw_group) {
		.hws = cc_hws + RTD1619B_CRT_PLL_BUS,
		.num_hws = ARRAY_SIZE(cc_hws) - RTD1619B_CRT_PLL_BUS,
	},
	.start_index = RTD1619B_CRT_PLL_BUS,
};

static int rtd1619b_cc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct rtk_clk_data *data;
	int ret;
	const struct clk_hw_map *map;

	map = of_device_get_match_data(dev);
	if (!map)
		return -EINVAL;

	data = rtk_clk_alloc_data(RTD1619B_CRT_CLK_MAX);
	if (!data)
		return -ENOMEM;

	ret = rtk_clk_of_init_data(np, data);
	if (ret) {
		rtk_clk_free_data(data);
		return ret;
	}

	of_rtk_clk_setup_crt(np, data->regmap);

	platform_set_drvdata(pdev, data);

	rtk_clk_add_hw_map(dev, data, map);

	rtk_clk_add_composites(dev, data, cc_composites,
			       ARRAY_SIZE(cc_composites));
	rtk_clk_add_gates(dev, data, cc_gates, ARRAY_SIZE(cc_gates));

	ret = of_clk_add_provider(np, of_clk_src_onecell_get, &data->clk_data);
	if (ret)
		dev_err(dev, "failed to add clk provider: %d\n", ret);

	cc_reset_initdata.lock = data->lock;
	cc_reset_initdata.regmap = data->regmap;
	rtk_reset_controller_add(dev, &cc_reset_initdata);

	return 0;
}

static const struct of_device_id rtd1619b_cc_match[] = {
	{ .compatible = "realtek,rtd1619b-crt-clk", .data = &rtd1619b_sys_map, },
	{ .compatible = "realtek,rtd1619b-crt-clk-n", .data = &rtd1619b_sys_n_map, },
	{ /* sentinel */ }
};

static struct platform_driver rtd1619b_cc_driver = {
	.probe = rtd1619b_cc_probe,
	.driver = {
		.name = "rtk-rtd1619b-crt-clk",
		.of_match_table = rtd1619b_cc_match,
	},
};

static int __init rtd1619b_cc_init(void)
{
	return platform_driver_register(&rtd1619b_cc_driver);
}
core_initcall(rtd1619b_cc_init);
MODULE_DESCRIPTION("Reatek RTD1619B CRT Controller Driver");
MODULE_AUTHOR("Cheng-Yu Lee <cylee12@realtek.com>");
MODULE_LICENSE("GPL v2");
