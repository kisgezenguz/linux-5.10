/*
 * Copyright (c) 2019 Realtek Semiconductor Corporation
 *
 * Author:
 *      Cheng-Yu Lee <cylee12@realtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __VE1_PM_H
#define __VE1_PM_H

struct device;

int ve_pd_init(struct device *dev);
void ve_pd_exit(struct device *dev);
int ve_pd_power_on(int no_reset);
int ve_pd_power_off(void);
int ve_pd_reset_control_reset(int idx);
int ve_pd_clk_set_parent(int idx, const char *parent_name);
int ve_pd_clk_parent_match(int idx, const char *clk_name);
int ve_pd_clk_set_rate(int idx, unsigned long rate);
unsigned long ve_pd_clk_get_rate(int idx);

#endif
