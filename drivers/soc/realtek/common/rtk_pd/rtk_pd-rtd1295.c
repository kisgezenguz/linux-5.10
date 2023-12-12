// SPDX-License-Identifier: GPL-2.0-only
/*
 * Power Controller of RTD-1295 SoC
 *
 * Copyright (C) 2017-2020 Realtek Semiconductor Corporation
 * Author: Cheng-Yu Lee <cylee12@realtek.com>
 */

#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <dt-bindings/power/rtd1295-power.h>
#include "rtk_pd_internal.h"

static struct rtk_pd pd_ve1 = {
	SET_PD_NAME("ve1"),
	SET_RTK_PD_SRAM_CONF_PWR5(0x380, 0x3a8, 0xf),
	SET_RTK_PD_ISO_CONF(0x400, 0),
};

static struct rtk_pd pd_ve2 = {
	SET_PD_NAME("ve2"),
	SET_RTK_PD_SRAM_CONF(0x3c0, 0xf),
	SET_RTK_PD_ISO_CONF(0x400, 4),
};

static struct rtk_pd pd_ve3 = {
	SET_PD_NAME("ve3"),
	SET_RTK_PD_SRAM_CONF(0x3e0, 0xf),
	SET_RTK_PD_ISO_CONF(0x400, 6),
};

static struct rtk_pd pd_gpu = {
	SET_PD_NAME("gpu"),
	SET_RTK_PD_SRAM_CONF_PWR5(0x394, 0x3ac, 0xf),
	SET_RTK_PD_ISO_CONF(0x400, 1),
};

static struct rtk_pd pd_nat = {
	SET_PD_NAME("nat"),
	SET_RTK_PD_SRAM_CONF(0x420, 0xf),
	SET_RTK_PD_ISO_CONF(0x400, 18),
};

static struct generic_pm_domain *rtd1295_domains[RTD1295_PD_MAX] = {
	[RTD1295_PD_VE1]      = rtk_pd_to_genpd(&pd_ve1),
	[RTD1295_PD_VE2]      = rtk_pd_to_genpd(&pd_ve2),
	[RTD1295_PD_VE3]      = rtk_pd_to_genpd(&pd_ve3),
	[RTD1295_PD_GPU]      = rtk_pd_to_genpd(&pd_gpu),
	[RTD1295_PD_NAT]      = rtk_pd_to_genpd(&pd_nat),
};

static int rtd1295_power_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct rtk_pd_device *pd_dev;
	int ret;

	pd_dev = devm_kzalloc(dev, sizeof(*pd_dev), GFP_KERNEL);
	if (!pd_dev)
		return -ENOMEM;

	pd_dev->dev = dev;
	pd_dev->regmap = syscon_node_to_regmap(np->parent);
	if (IS_ERR(pd_dev->regmap)) {
		ret = PTR_ERR(pd_dev->regmap);
		dev_err(dev, "failed to get syscon: %d\n", ret);
		return ret;
	}
	INIT_LIST_HEAD(&pd_dev->list);

	rtk_pd_device_add_domains(pd_dev, rtd1295_domains, ARRAY_SIZE(rtd1295_domains));

	pd_dev->of_provider_data.domains = rtd1295_domains;
	pd_dev->of_provider_data.num_domains = ARRAY_SIZE(rtd1295_domains);
	ret = of_genpd_add_provider_onecell(np, &pd_dev->of_provider_data);
	WARN(ret, "of_genpd_add_provider_onecell() returns %d\n", ret);

	dev_set_drvdata(dev, pd_dev);
	return 0;
}

static const struct of_device_id rtd1295_power_match[] = {
	{ .compatible = "realtek,rtd1295-power" },
	{}
};

static struct platform_driver rtd1295_power_driver = {
	.probe = rtd1295_power_probe,
	.driver = {
		.name = "rtk-rtd1295-power",
		.of_match_table = of_match_ptr(rtd1295_power_match),
		.pm = &rtk_pd_generic_pm_ops,
	},
};

static int __init rtd1295_power_init(void)
{
	return platform_driver_register(&rtd1295_power_driver);
}
arch_initcall(rtd1295_power_init);

MODULE_DESCRIPTION("Realtek RTD1295 Power Controller");
MODULE_AUTHOR("Cheng-Yu Lee <cylee12@realtek.com>");
MODULE_LICENSE("GPL v2");

