/*
 * arch/arm/mach-tegra/board-betelgeuse.h
 *
 * Copyright (C) 2010 Google, Inc.
 *               2011 Artem Makhutov <artem@makhutov.org>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _MACH_TEGRA_BOARD_BETELGEUSE_H
#define _MACH_TEGRA_BOARD_BETELGEUSE_H

void betelgeuse_pinmux_init(void);
int betelgeuse_power_init(void);
int betelgeuse_panel_init(void);
int betelgeuse_sdhci_init(void);

/* TPS6586X gpios */
#define TPS6586X_GPIO_BASE      TEGRA_NR_GPIOS
#define AVDD_DSI_CSI_ENB_GPIO   (TPS6586X_GPIO_BASE + 1) /* gpio2 */

/* WM8903 gpios */
#define WM8903_GPIO_BASE        (TEGRA_NR_GPIOS + 32)
#define WM8903_GP1              (WM8903_GPIO_BASE + 0)
#define WM8903_GP2              (WM8903_GPIO_BASE + 1)
#define WM8903_GP3              (WM8903_GPIO_BASE + 2)
#define WM8903_GP4              (WM8903_GPIO_BASE + 3)
#define WM8903_GP5              (WM8903_GPIO_BASE + 4)

/* Interrupt numbers from external peripherals */
#define TPS6586X_INT_BASE       TEGRA_NR_IRQS
#define TPS6586X_INT_END        (TPS6586X_INT_BASE + 32)

#define TEGRA_GPIO_USB1		TEGRA_GPIO_PU3
#define TEGRA_GPIO_MAGNETOMETER	TEGRA_GPIO_PV1
#define TEGRA_GPIO_CDC_IRQ	TEGRA_GPIO_PW2

#define SHUTTLE_FB_PAGES        2			/* At least, 2 video pages */
#define SHUTTLE_FB_HDMI_PAGES   2			/* At least, 2 video pages for HDMI */

#define SHUTTLE_MEM_SIZE	SZ_512M			/* Total memory */
#define SHUTTLE_GPU_MEM_SIZE	SZ_128M			/* Memory reserved for GPU */

#endif
