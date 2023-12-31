// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017 Realtek Semiconductor Corp.
 * Copyright (c) 2020 Realtek Semiconductor Corp.
 *
 */

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/usb/otg.h>

#include "rtk_usb.h"

struct rtk_usb {
	struct power_ctrl_reg {
		/* l4_icg */
		u32 p0_l4_icg;
		u32 p1_l4_icg;
		u32 p2_l4_icg;
	} power_ctrl_reg;

	bool usb_power_cut;

	struct type_c {
		struct gpio_desc *connector_switch_gpio;
		struct gpio_desc *plug_side_switch_gpio;
	} type_c;

	u32 pcie_usb3phy_sel;
};

#define disable_cc 0x0
#define enable_cc1 0x1
#define enable_cc2 0x2

static struct rtk_usb *rtk_usb;

/* set usb charger power */
static void usb_set_charger_power(struct rtk_usb *rtk_usb, unsigned int val)
{
	if (!rtk_usb)
		return;
}

static int usb_set_hw_l4icg_on_off(struct rtk_usb *rtk_usb,
	    enum usb_port_num port_num, bool on)
{
	void __iomem *reg;

	if (!rtk_usb)
		return 0;

	switch (port_num) {
	case USB_PORT_0:
		if (rtk_usb->power_ctrl_reg.p0_l4_icg) {
			pr_info("%s set l4_icg port %d\n", __func__, port_num);
			reg = ioremap(rtk_usb->power_ctrl_reg.p0_l4_icg, 0x4);
			writel((on&BIT(0)) | readl(reg), reg);
			iounmap(reg);
		}
		break;
	case USB_PORT_1:
		if (rtk_usb->power_ctrl_reg.p1_l4_icg) {
			pr_info("%s set l4_icg port %d\n", __func__, port_num);
			reg = ioremap(rtk_usb->power_ctrl_reg.p1_l4_icg, 0x4);
			writel((on&BIT(0)) | readl(reg), reg);
			iounmap(reg);
		}
		break;
	case USB_PORT_2:
		if (rtk_usb->power_ctrl_reg.p2_l4_icg) {
			pr_info("%s set l4_icg port %d\n", __func__, port_num);
			reg = ioremap(rtk_usb->power_ctrl_reg.p2_l4_icg, 0x4);
			writel((on&BIT(0)) | readl(reg), reg);
			iounmap(reg);
		}
		break;
	default:
		pr_err("%s Error Port num %d\n", __func__, port_num);
		break;
	}

	return 0;
}

static int usb_iso_power_ctrl(struct rtk_usb *rtk_usb,
	    bool power_on)
{
	if (!rtk_usb)
		return 0;

	pr_debug("%s START\n", __func__);

	pr_debug("%s END\n", __func__);
	return 0;
}

static int usb_port_suspend_resume(struct rtk_usb *rtk_usb,
	    enum usb_port_num port_num, bool is_suspend)
{
	if (!rtk_usb)
		return 0;

	return 0;
}

static int type_c_init(struct rtk_usb *rtk_usb)
{
	if (!rtk_usb)
		return 0;

	return 0;
}

static int type_c_plug_config(struct rtk_usb *rtk_usb, int dr_mode, int cc)
{
	bool high;

	/* host / device */
	if (dr_mode == USB_DR_MODE_PERIPHERAL)
		high = true;
	else if (dr_mode == USB_DR_MODE_HOST)
		high = false;
	else
		goto cc_config;

	if (!IS_ERR(rtk_usb->type_c.connector_switch_gpio)) {
		pr_info("%s Set connector to %s by gpio %d\n",
			    __func__, high?"device":"host",
			    desc_to_gpio(rtk_usb->type_c.connector_switch_gpio));
		if (gpiod_direction_output(
			    rtk_usb->type_c.connector_switch_gpio, high))
			pr_err("%s ERROR connector_switch_ctrl_gpio fail\n",
				    __func__);
	}

cc_config:
	/* cc pin */
	/* host / device */
	if (cc == enable_cc1)
		high = true;
	else if (cc == enable_cc2)
		high = false;
	else
		goto out;

	if (!IS_ERR(rtk_usb->type_c.plug_side_switch_gpio)) {
		pr_info("%s type plug to %s by gpio %d\n",
			    __func__, high?"cc1":"cc2",
			    desc_to_gpio(rtk_usb->type_c.plug_side_switch_gpio));
		if (gpiod_direction_output(
			    rtk_usb->type_c.plug_side_switch_gpio, high))
			pr_err("%s set ERROR plug_side_switch_gpio fail\n",
				    __func__);
	}

out:
	return 0;
}

static struct rtk_usb *usb_soc_init(struct device_node *node)
{
	struct device_node *sub_node;

	if (!rtk_usb)
		rtk_usb = kzalloc(sizeof(struct rtk_usb), GFP_KERNEL);

	pr_info("%s START (%s)\n", __func__, __FILE__);

	sub_node = of_get_child_by_name(node, "power_ctrl_reg");
	if (sub_node) {
		pr_debug("%s sub_node %s\n", __func__, sub_node->name);

		of_property_read_u32(sub_node,
			    "p0_l4_icg", &rtk_usb->power_ctrl_reg.p0_l4_icg);
		of_property_read_u32(sub_node,
			    "p1_l4_icg", &rtk_usb->power_ctrl_reg.p1_l4_icg);
		of_property_read_u32(sub_node,
			    "p2_l4_icg", &rtk_usb->power_ctrl_reg.p2_l4_icg);

		if (of_property_read_bool(sub_node, "usb_power_cut"))
			rtk_usb->usb_power_cut = true;
		else
			rtk_usb->usb_power_cut = false;
	}

	sub_node = of_get_child_by_name(node, "type_c");
	if (sub_node) {

		pr_debug("%s sub_node %s\n", __func__, sub_node->name);

		rtk_usb->type_c.connector_switch_gpio = gpiod_get_from_of_node(sub_node, "realtek,connector_switch-gpio",
										0, GPIOD_OUT_HIGH, "usb-connector_switch");
		if (IS_ERR(rtk_usb->type_c.connector_switch_gpio)) {
			pr_info("connector_switch-gpio no found");
		} else {
			pr_info("%s get connector_switch-gpio (id=%d) OK\n",
				    __func__, desc_to_gpio(rtk_usb->type_c.connector_switch_gpio));
		}

		rtk_usb->type_c.plug_side_switch_gpio = gpiod_get_from_of_node(sub_node, "realtek,plug_side_switch-gpio",
										0, GPIOD_OUT_HIGH, "usb-plug_side_switch");
		if (IS_ERR(rtk_usb->type_c.plug_side_switch_gpio)) {
			pr_info("plug_side_switch-gpio no found");
		} else {
			pr_info("%s get plug_side_switch-gpio (id=%d) OK\n",
				    __func__, desc_to_gpio(rtk_usb->type_c.plug_side_switch_gpio));
		}
	}
	if (of_property_read_u32(node,
			    "pcie_usb3phy_sel", &rtk_usb->pcie_usb3phy_sel)) {
		rtk_usb->pcie_usb3phy_sel = -1;
	} else {
		void __iomem *reg = ioremap(rtk_usb->pcie_usb3phy_sel, 0x4);
		int val = readl(reg) | BIT(6);

		writel(val, reg);

		pr_info("%s: pcie_usb3phy_sel=%x to enable usb3phy (val=0x%x)\n",
			    __func__, rtk_usb->pcie_usb3phy_sel, readl(reg));

		iounmap(reg);
	}

	pr_info("%s END\n", __func__);
	return rtk_usb;
}

static int usb_soc_free(struct rtk_usb **rtk_usb)
{
	if (*rtk_usb) {
		kfree(*rtk_usb);
		*rtk_usb = NULL;
	}
	return 0;
}

int rtk_usb_rtd13xx_init(struct rtk_usb_ops *ops)
{

	if (ops == NULL)
		return -1;

	ops->usb_soc_init = &usb_soc_init;
	ops->usb_soc_free = &usb_soc_free;

	ops->usb_port_suspend_resume = &usb_port_suspend_resume;
	ops->usb_set_hw_l4icg_on_off = &usb_set_hw_l4icg_on_off;

	ops->usb_iso_power_ctrl = &usb_iso_power_ctrl;

	ops->usb_set_charger_power = &usb_set_charger_power;

	/* For Type c */
	ops->type_c_init = &type_c_init;
	ops->type_c_plug_config = &type_c_plug_config;

	return 0;
}
