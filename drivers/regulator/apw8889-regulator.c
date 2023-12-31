// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2019 Realtek Semiconductor Corporation
 * Author: Cheng-Yu Lee <cylee12@realtek.com>
 */

#include <linux/list.h>
#include <linux/mfd/apw8889.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/suspend.h>
#include "apw888x-regulator.h"

/* regulator id */
enum {
	APW8889_ID_DC1 = 0,
	APW8889_ID_DC2,
	APW8889_ID_DC3,
	APW8889_ID_DC4,
	APW8889_ID_DC5,
	APW8889_ID_DC6,
	APW8889_ID_LDO1,
	APW8889_ID_MAX
};

#define APW8889_DESC(_id, _name, _min_uV, _uV_step, _n_volt, _type)\
{                                                                  \
	.desc = {                                                  \
		.owner       = THIS_MODULE,                        \
		.type        = REGULATOR_VOLTAGE,                  \
		.ops         = &apw888x_regulator_ops,             \
		.name        = _name,                              \
		.n_voltages  = _n_volt,                            \
		.min_uV      = _min_uV,                            \
		.uV_step     = _uV_step,                           \
		.id          = APW8889_ID_ ## _id,                 \
		.vsel_reg    = APW8889_REG_ ## _id ## _NRMVOLT,    \
		.vsel_mask   = APW8889_ ## _id ## _NRMVOLT_MASK,   \
		.enable_reg  = APW8889_REG_ONOFF,                  \
		.enable_mask = APW8889_ ## _id ## _ON_MASK,        \
		.enable_val  = APW8889_ ## _id ## _ON_MASK,        \
		.of_map_mode = apw888x_regulator_ ## _type ##_of_map_mode, \
	},                                                         \
	.nmode_reg  = APW8889_REG_ ## _id ## _MODE,                \
	.nmode_mask = APW8889_ ## _id ## _NRMMODE_MASK,            \
	.smode_reg  = APW8889_REG_ ## _id ## _MODE,                \
	.smode_mask = APW8889_ ## _id ## _SLPMODE_MASK,            \
	.svsel_reg  = APW8889_REG_ ## _id ## _SLPVOLT,             \
	.svsel_mask = APW8889_ ## _id ## _SLPVOLT_MASK,            \
	.dischg_reg = APW8889_REG_DISCHG,                          \
	.dischg_mask = APW8889_ ## _id ## _DISCHG_MASK,            \
}

#define APW8889_DESC_CLP(_id, _name, _min_uV, _uV_step, _n_volt, _type) \
{                                                                  \
	.desc = {                                                  \
		.owner       = THIS_MODULE,                        \
		.type        = REGULATOR_VOLTAGE,                  \
		.ops         = &apw888x_regulator_ops,             \
		.name        = _name,                              \
		.n_voltages  = _n_volt,                            \
		.min_uV      = _min_uV,                            \
		.uV_step     = _uV_step,                           \
		.id          = APW8889_ID_ ## _id,                 \
		.vsel_reg    = APW8889_REG_ ## _id ## _NRMVOLT,    \
		.vsel_mask   = APW8889_ ## _id ## _NRMVOLT_MASK,   \
		.enable_reg  = APW8889_REG_ONOFF,                  \
		.enable_mask = APW8889_ ## _id ## _ON_MASK,        \
		.enable_val  = APW8889_ ## _id ## _ON_MASK,        \
		.of_map_mode = apw888x_regulator_ ## _type ##_of_map_mode, \
	},                                                         \
	.nmode_reg  = APW8889_REG_ ## _id ## _MODE,                \
	.nmode_mask = APW8889_ ## _id ## _NRMMODE_MASK,            \
	.smode_reg  = APW8889_REG_ ## _id ## _MODE,                \
	.smode_mask = APW8889_ ## _id ## _SLPMODE_MASK,            \
	.svsel_reg  = APW8889_REG_ ## _id ## _SLPVOLT,             \
	.svsel_mask = APW8889_ ## _id ## _SLPVOLT_MASK,            \
	.dischg_reg = APW8889_REG_DISCHG,                          \
	.dischg_mask = APW8889_ ## _id ## _DISCHG_MASK,            \
}

#define APW8889_DESC_FIXED_UV(_id, _name, _fixed_uV)               \
{                                                                  \
	.desc = {                                                  \
		.owner       = THIS_MODULE,                        \
		.type        = REGULATOR_VOLTAGE,                  \
		.ops         = &apw888x_regulator_fixed_uV_ops,    \
		.name        = _name,                              \
		.n_voltages  = 1,                                  \
		.fixed_uV    = _fixed_uV,                          \
		.id          = APW8889_ID_ ## _id,                 \
		.enable_reg  = APW8889_REG_ONOFF,                  \
		.enable_mask = APW8889_ ## _id ## _ON_MASK,        \
		.enable_val  = APW8889_ ## _id ## _ON_MASK,        \
		.of_map_mode = apw888x_regulator_dc_of_map_mode,   \
	},                                                         \
	.nmode_reg  = APW8889_REG_ ## _id ## _MODE,                \
	.nmode_mask = APW8889_ ## _id ## _NRMMODE_MASK,            \
	.smode_reg  = APW8889_REG_ ## _id ## _MODE,                \
	.smode_mask = APW8889_ ## _id ## _SLPMODE_MASK,            \
	.dischg_reg = APW8889_REG_DISCHG,                          \
	.dischg_mask = APW8889_ ## _id ## _DISCHG_MASK,            \
}

static struct apw888x_regulator_desc desc[] = {
	[APW8889_ID_DC1]  = APW8889_DESC(DC1, "dc1", 2200000, 25000, 64, dc),
	[APW8889_ID_DC2]  = APW8889_DESC_CLP(DC2, "dc2", 550000, 12500, 64, dc),
	[APW8889_ID_DC3]  = APW8889_DESC_CLP(DC3, "dc3", 550000, 12500, 64, dc),
	[APW8889_ID_DC4]  = APW8889_DESC_CLP(DC4, "dc4", 550000, 12500, 64, dc),
	[APW8889_ID_DC5]  = APW8889_DESC_FIXED_UV(DC5, "dc5", 0),
	[APW8889_ID_DC6]  = APW8889_DESC(DC6, "dc6",  800000, 20000, 64, dc),
	[APW8889_ID_LDO1] = APW8889_DESC(LDO1, "ldo1", 1780000, 40000, 32, ldo),
};

static int apw8889_regulator_suspend(struct device *dev)
{
	struct apw888x_regulator_device *regdev = dev_get_drvdata(dev);

	dev_info(dev, "enter %s\n", __func__);
	apw888x_regulator_device_generic_suspend(regdev);
	dev_info(dev, "exit %s\n", __func__);
	return 0;
}

static int apw8889_regulator_resume(struct device *dev)
{
	struct apw888x_regulator_device *regdev = dev_get_drvdata(dev);

	dev_info(dev, "enter %s\n", __func__);
	apw888x_regulator_device_generic_resume(regdev);
	dev_info(dev, "exit %s\n", __func__);
	return 0;
}

static const struct dev_pm_ops apw8889_regulator_pm_ops = {
	.suspend = apw8889_regulator_suspend,
	.resume  = apw8889_regulator_resume,
};

static int apw8889_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct apw888x_regulator_device *regdev;
	int i, ret;

	regdev = devm_kzalloc(dev, sizeof(*regdev), GFP_KERNEL);
	if (!regdev)
		return -ENOMEM;

	regdev->regmap = dev_get_regmap(dev->parent, NULL);
	if (!regdev->regmap)
		return -EINVAL;

	regdev->dev = dev;
	INIT_LIST_HEAD(&regdev->list);

	regcache_cache_bypass(regdev->regmap, true);
	ret = regmap_write(regdev->regmap, APW8889_REG_DISCHG, 0xfd);
	if (ret)
		dev_warn(dev, "failed to reset dischg: %d\n", ret);
	regcache_cache_bypass(regdev->regmap, false);

	for (i = 0; i < ARRAY_SIZE(desc); i++) {
		ret = apw888x_regulator_register(regdev, &desc[i]);
		if (ret) {
			dev_err(dev, "failed to register %s: %d\n",
				desc[i].desc.name, ret);
			return ret;
		}
	}

	platform_set_drvdata(pdev, regdev);
	return 0;
}

static void apw8889_regulator_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct apw888x_regulator_device *regdev = platform_get_drvdata(pdev);

	dev_info(dev, "enter %s\n", __func__);
	apw888x_regulator_device_generic_shutdown(regdev);
	dev_info(dev, "exit %s\n", __func__);
}

static const struct of_device_id apw8889_regulator_ids[] = {
	{ .compatible = "anpec,apw8889-regulator", },
	{}
};
MODULE_DEVICE_TABLE(of, apw8889_regulator_ids);

static struct platform_driver apw8889_regulator_driver = {
	.driver = {
		.name = "apw8889-regulator",
		.owner = THIS_MODULE,
		.pm = &apw8889_regulator_pm_ops,
		.of_match_table = apw8889_regulator_ids,
	},
	.probe    = apw8889_regulator_probe,
	.shutdown = apw8889_regulator_shutdown,
};
module_platform_driver(apw8889_regulator_driver);

MODULE_DESCRIPTION("Anpec APW8889 Regulator Driver");
MODULE_AUTHOR("Cheng-Yu Lee <cylee12@realtek.com>");
MODULE_LICENSE("GPL vw");


