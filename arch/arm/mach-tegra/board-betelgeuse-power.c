/*
 * Copyright (C) 2010 NVIDIA, Inc.
 *               2010 Marc Dietrich <marvin24@gmx.de>
 *               2011 Artem Makhutov <artem@makhutov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */
#include <linux/i2c.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/tps6586x.h>
#include <linux/gpio.h>
//#include <linux/power/gpio-charger.h>
#include <linux/platform_device.h>
#include <linux/err.h>

#include "board-betelgeuse.h"
#include "gpio-names.h"

static struct regulator_consumer_supply tps658621_sm0_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL),
};
static struct regulator_consumer_supply tps658621_sm1_supply[] = {
	REGULATOR_SUPPLY("vdd_cpu", NULL),
};
static struct regulator_consumer_supply tps658621_sm2_supply[] = {
	REGULATOR_SUPPLY("vdd_sm2", NULL),
};
static struct regulator_consumer_supply tps658621_ldo0_supply[] = { /* VDDIO_PEX_CLK */
	REGULATOR_SUPPLY("pex_clk", NULL),
};
static struct regulator_consumer_supply tps658621_ldo1_supply[] = { /* 1V2 */
	REGULATOR_SUPPLY("pll_a", NULL),
	REGULATOR_SUPPLY("pll_m", NULL),
	REGULATOR_SUPPLY("pll_p", NULL),
	REGULATOR_SUPPLY("pll_c", NULL),
	REGULATOR_SUPPLY("pll_u", NULL),
	REGULATOR_SUPPLY("pll_u1", NULL),
	REGULATOR_SUPPLY("pll_s", NULL),
	REGULATOR_SUPPLY("pll_x", NULL),
};
static struct regulator_consumer_supply tps658621_ldo2_supply[] = { /* VDD_RTC */
	REGULATOR_SUPPLY("vdd_rtc", NULL),
	REGULATOR_SUPPLY("vdd_aon", NULL),
};
static struct regulator_consumer_supply tps658621_ldo3_supply[] = { /* 3V3 */
	REGULATOR_SUPPLY("avdd_usb", NULL),
	REGULATOR_SUPPLY("avdd_usb_pll", NULL),
	REGULATOR_SUPPLY("vddio_nand_3v3", NULL), /* AON? */
	REGULATOR_SUPPLY("sdio", NULL),
	REGULATOR_SUPPLY("vmmc", NULL),
	REGULATOR_SUPPLY("vddio_vi", NULL),
	REGULATOR_SUPPLY("avdd_lvds", NULL),
	REGULATOR_SUPPLY("tmon0", NULL),
	REGULATOR_SUPPLY("vddio_wlan", NULL),
};
static struct regulator_consumer_supply tps658621_ldo4_supply[] = { 
	REGULATOR_SUPPLY("avdd_osc", NULL),       /* AVDD_OSC */
	REGULATOR_SUPPLY("vddio_sys", NULL),
	REGULATOR_SUPPLY("vddio_lcd", NULL),      /* AON? */
	REGULATOR_SUPPLY("vddio_audio", NULL),    /* AON? */
	REGULATOR_SUPPLY("vddio_ddr", NULL),      /* AON? */
	REGULATOR_SUPPLY("vddio_uart", NULL),     /* AON? */
	REGULATOR_SUPPLY("vddio_bb", NULL),       /* AON? */
	REGULATOR_SUPPLY("tmon1.8vs", NULL),
	REGULATOR_SUPPLY("vddhostif_bt", NULL),
	REGULATOR_SUPPLY("wifi3vs", NULL),
};
static struct regulator_consumer_supply tps658621_ldo5_supply[] = {
	REGULATOR_SUPPLY("vddio_nand", NULL),
};
static struct regulator_consumer_supply tps658621_ldo6_supply[] = {
	REGULATOR_SUPPLY("avdd_vdac", NULL),
};
static struct regulator_consumer_supply tps658621_ldo7_supply[] = {
	REGULATOR_SUPPLY("avdd_hdmi", NULL),
};
static struct regulator_consumer_supply tps658621_ldo8_supply[] = { /* AVDD_HDMI_PLL */
	REGULATOR_SUPPLY("avdd_hdmi_pll", NULL),  /* PLLHD */
};
static struct regulator_consumer_supply tps658621_ldo9_supply[] = {
	REGULATOR_SUPPLY("vdd_ddr_rx", NULL),
};

/*
 * Skip these for new as they require some more work
 *
static struct regulator_consumer_supply tps658621_buck_supply[] = {
	REGULATOR_SUPPLY("pll_e", NULL),
};
static struct regulator_consumer_supply tps658621_soc_supply[] = {
	REGULATOR_SUPPLY("soc", NULL),
	REGULATOR_SUPPLY("pex_clk", NULL),
};
*/

#define REGULATOR_INIT(_id, _minmv, _maxmv)				\
	{								\
		.constraints = {					\
			.min_uV = (_minmv)*1000,			\
			.max_uV = (_maxmv)*1000,			\
			.valid_modes_mask = (REGULATOR_MODE_NORMAL |	\
					     REGULATOR_MODE_STANDBY),	\
			.valid_ops_mask = (REGULATOR_CHANGE_MODE |	\
					   REGULATOR_CHANGE_STATUS |	\
					   REGULATOR_CHANGE_VOLTAGE),	\
		},							\
		.num_consumer_supplies = ARRAY_SIZE(tps658621_##_id##_supply),\
		.consumer_supplies = tps658621_##_id##_supply,		\
	}

static struct regulator_init_data sm0_data = REGULATOR_INIT(sm0, 625, 2700);    // 1200
static struct regulator_init_data sm1_data = REGULATOR_INIT(sm1, 625, 1100);    // 1000
static struct regulator_init_data sm2_data = REGULATOR_INIT(sm2, 3000, 4550);   // 3700
static struct regulator_init_data ldo0_data = REGULATOR_INIT(ldo0, 1250, 3300); // 3300
static struct regulator_init_data ldo1_data = REGULATOR_INIT(ldo1, 725, 1500);  // 1100
static struct regulator_init_data ldo2_data = REGULATOR_INIT(ldo2, 725, 1500);  // 1200
static struct regulator_init_data ldo3_data = REGULATOR_INIT(ldo3, 1250, 3300); // 3300
static struct regulator_init_data ldo4_data = REGULATOR_INIT(ldo4, 1700, 2000); // 1800
static struct regulator_init_data ldo5_data = REGULATOR_INIT(ldo5, 1250, 3300); // 2850
static struct regulator_init_data ldo6_data = REGULATOR_INIT(ldo6, 1250, 3300); // 2850
static struct regulator_init_data ldo7_data = REGULATOR_INIT(ldo7, 1250, 3300); // 3300
static struct regulator_init_data ldo8_data = REGULATOR_INIT(ldo8, 1250, 3300); // 1800
static struct regulator_init_data ldo9_data = REGULATOR_INIT(ldo9, 1250, 3300); // 2850
/*
static struct regulator_init_data soc_data = REGULATOR_INIT(soc, 1250, 3300);
static struct regulator_init_data buck_data = REGULATOR_INIT(buck, 1250, 3300); 
*/

#define TPS_REG(_id, _data)			\
	{					\
		.id = TPS6586X_ID_##_id,	\
		.name = "tps6586x-regulator",	\
		.platform_data = _data,		\
	}

/* FIXME: do we have rtc alarm irq? */
static struct tps6586x_rtc_platform_data betelgeuse_rtc_data = {
/*	.irq	= TEGRA_NR_IRQS + TPS6586X_INT_RTC_ALM1, */
	.irq	= -1,
	.start	= {
			.year	= 2009,
			.month	= 1,
			.day	= 1,
			.hour	= 0,
			.min	= 0,
			.sec	= 0,
		},
};

static struct tps6586x_subdev_info tps_devs[] = {
	TPS_REG(SM_0, &sm0_data),
	TPS_REG(SM_1, &sm1_data),
	TPS_REG(SM_2, &sm2_data),
	TPS_REG(LDO_0, &ldo0_data),
	TPS_REG(LDO_1, &ldo1_data),
	TPS_REG(LDO_2, &ldo2_data),
	TPS_REG(LDO_3, &ldo3_data),
	TPS_REG(LDO_4, &ldo4_data),
	TPS_REG(LDO_5, &ldo5_data),
	TPS_REG(LDO_6, &ldo6_data),
	TPS_REG(LDO_7, &ldo7_data),
	TPS_REG(LDO_8, &ldo8_data),
	TPS_REG(LDO_9, &ldo9_data),
/*	TPS_REG(SOC, &soc_data),
	TPS_REG(BUCK, &buck_data), */
	{
		.id		= 0,
		.name		= "tps6586x-rtc",
		.platform_data	= &betelgeuse_rtc_data,
	},
};

static struct tps6586x_platform_data tps_platform = {
	.num_subdevs = ARRAY_SIZE(tps_devs),
	.subdevs = tps_devs,
	.gpio_base = TEGRA_NR_GPIOS,
};

static struct i2c_board_info __initdata betelgeuse_regulators[] = {
	{
		I2C_BOARD_INFO("tps6586x", 0x34),
		.platform_data = &tps_platform,
	},
};

int __init betelgeuse_regulator_init(void)
{
	i2c_register_board_info(4, betelgeuse_regulators, 1);
	return 0;
}

int __init betelgeuse_power_init(void)
{
	int err;

	err = betelgeuse_regulator_init();
	if (err < 0) {
		pr_warning("Unable to initialize regulator\n");
		return -1;
	}

	return 0;
}
