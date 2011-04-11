/*
 * arch/arm/mach-tegra/board-betelgeuse-panel.c
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 *               2011, Artem Makhutov <artem@makhutov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/pwm_backlight.h>
#include <mach/nvhost.h>
#include <mach/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>

#include "devices.h"
#include "gpio-names.h"

#define betelgeuse_bl_enb		TEGRA_GPIO_PB5
#define betelgeuse_lvds_shutdown	TEGRA_GPIO_PB2
#define betelgeuse_en_vdd_pnl	TEGRA_GPIO_PC6
#define betelgeuse_bl_vdd		TEGRA_GPIO_PW0
#define betelgeuse_bl_pwm		TEGRA_GPIO_PB4

static int betelgeuse_backlight_init(struct device *dev)
{
	int ret;

	ret = gpio_request(betelgeuse_bl_enb, "backlight_enb");
	if (ret < 0)
		return ret;

	ret = gpio_direction_output(betelgeuse_bl_enb, 1);
	if (ret < 0)
		gpio_free(betelgeuse_bl_enb);
	else
		tegra_gpio_enable(betelgeuse_bl_enb);

	return ret;
};

static void betelgeuse_backlight_exit(struct device *dev)
{
	gpio_set_value(betelgeuse_bl_enb, 0);
	gpio_free(betelgeuse_bl_enb);
	tegra_gpio_disable(betelgeuse_bl_enb);
}

static int betelgeuse_backlight_notify(struct device *unused, int brightness)
{
	gpio_set_value(betelgeuse_en_vdd_pnl, !!brightness);
	gpio_set_value(betelgeuse_lvds_shutdown, !!brightness);
	gpio_set_value(betelgeuse_bl_enb, !!brightness);
	return brightness;
}

static struct platform_pwm_backlight_data betelgeuse_backlight_data = {
	.pwm_id		= 0,
	.max_brightness	= 255,
	.dft_brightness	= 224,
	.pwm_period_ns	= 5000000,
	.init		= betelgeuse_backlight_init,
	.exit		= betelgeuse_backlight_exit,
	.notify		= betelgeuse_backlight_notify,
};

static struct platform_device betelgeuse_backlight_device = {
	.name	= "pwm-backlight",
	.id	= -1,
	.dev	= {
		.platform_data = &betelgeuse_backlight_data,
	},
};

static int betelgeuse_panel_enable(void)
{
	gpio_set_value(betelgeuse_lvds_shutdown, 1);
	return 0;
}

static int betelgeuse_panel_disable(void)
{
	gpio_set_value(betelgeuse_lvds_shutdown, 0);
	return 0;
}

static struct resource betelgeuse_disp1_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_GENERAL,
		.end	= INT_DISPLAY_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY_BASE,
		.end	= TEGRA_DISPLAY_BASE + TEGRA_DISPLAY_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= 0x1c012000,
		.end	= 0x1c012000 + 0x258000 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct tegra_dc_mode betelgeuse_panel_modes[] = {
	{
		.pclk = 62200000,
		.h_ref_to_sync = 11,
		.v_ref_to_sync = 1,
		.h_sync_width = 26,
		.v_sync_width = 6,
		.h_back_porch = 12,
		.v_back_porch = 3,
		.h_active = 1024,
		.v_active = 600,
		.h_front_porch = 45,
		.v_front_porch = 3,
	},
};

static struct tegra_fb_data betelgeuse_fb_data = {
	.win		= 0,
	.xres		= 1024,
	.yres		= 600,
	.bits_per_pixel	= 16,
};

static struct tegra_dc_out betelgeuse_disp1_out = {
	.type		= TEGRA_DC_OUT_RGB,

	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.modes		= betelgeuse_panel_modes,
	.n_modes	= ARRAY_SIZE(betelgeuse_panel_modes),

	.enable		= betelgeuse_panel_enable,
	.disable	= betelgeuse_panel_disable,
};

static struct tegra_dc_platform_data betelgeuse_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &betelgeuse_disp1_out,
	.fb		= &betelgeuse_fb_data,
};

static struct nvhost_device betelgeuse_disp1_device = {
	.name		= "tegradc",
	.id		= 0,
	.resource	= betelgeuse_disp1_resources,
	.num_resources	= ARRAY_SIZE(betelgeuse_disp1_resources),
	.dev = {
		.platform_data = &betelgeuse_disp1_pdata,
	},
};

static struct nvmap_platform_carveout betelgeuse_carveouts[] = {
	[0] = {
		.name		= "iram",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_IRAM,
		.base		= TEGRA_IRAM_BASE,
		.size		= TEGRA_IRAM_SIZE,
		.buddy_size	= 0, /* no buddy allocation for IRAM */
	},
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= 0x18C00000,
		.size		= SZ_128M - 0xC00000,
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data betelgeuse_nvmap_data = {
	.carveouts	= betelgeuse_carveouts,
	.nr_carveouts	= ARRAY_SIZE(betelgeuse_carveouts),
};

static struct platform_device betelgeuse_nvmap_device = {
	.name	= "tegra-nvmap",
	.id	= -1,
	.dev	= {
		.platform_data = &betelgeuse_nvmap_data,
	},
};

static struct platform_device *betelgeuse_gfx_devices[] __initdata = {
	&betelgeuse_nvmap_device,
	&tegra_grhost_device,
	&tegra_pwfm0_device,
	&betelgeuse_backlight_device,
};

int __init betelgeuse_panel_init(void)
{
	int err;

	gpio_request(betelgeuse_en_vdd_pnl, "en_vdd_pnl");
	gpio_direction_output(betelgeuse_en_vdd_pnl, 1);
	tegra_gpio_enable(betelgeuse_en_vdd_pnl);

	gpio_request(betelgeuse_bl_vdd, "bl_vdd");
	gpio_direction_output(betelgeuse_bl_vdd, 1);
	tegra_gpio_enable(betelgeuse_bl_vdd);

	gpio_request(betelgeuse_lvds_shutdown, "lvds_shdn");
	gpio_direction_output(betelgeuse_lvds_shutdown, 1);
	tegra_gpio_enable(betelgeuse_lvds_shutdown);

	err = platform_add_devices(betelgeuse_gfx_devices,
				   ARRAY_SIZE(betelgeuse_gfx_devices));

	if (!err)
		err = nvhost_device_register(&betelgeuse_disp1_device);

	return err;
}

