/*
 * arch/arm/mach-tegra/board-betelgeuse.c
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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/pda_power.h>
#include <linux/i2c.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/usb/android_composite.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>

#include <mach/audio.h>
#include <mach/i2s.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/clk.h>
#include <mach/usb_phy.h>
#include <mach/suspend.h>
#include <mach/spdif.h>
#include <mach/tegra_das.h>

#include <sound/wm8903.h>

#include "clock.h"
#include "board.h"
#include "board-betelgeuse.h"
#include "devices.h"
#include "gpio-names.h"

/* NVidia bootloader tags */
#define ATAG_NVIDIA		0x41000801

#define ATAG_NVIDIA_RM			0x1
#define ATAG_NVIDIA_DISPLAY		0x2
#define ATAG_NVIDIA_FRAMEBUFFER		0x3
#define ATAG_NVIDIA_CHIPSHMOO		0x4
#define ATAG_NVIDIA_CHIPSHMOOPHYS	0x5
#define ATAG_NVIDIA_PRESERVED_MEM_0	0x10000
#define ATAG_NVIDIA_PRESERVED_MEM_N	2
#define ATAG_NVIDIA_FORCE_32		0x7fffffff

//static char *usb_functions[] = { "mtp" };
//static char *usb_functions_adb[] = { "acm", "mtp", "adb" };
static char *usb_functions_adb[] = { "usb_mass_storage", "adb", "acm" };

static struct android_usb_product usb_products[] = {
//        {
//                .product_id     = 0x7102,
//                .num_functions  = ARRAY_SIZE(usb_functions),
//                .functions      = usb_functions,
//        },
        {
                .product_id     = 0x7100,
                .num_functions  = ARRAY_SIZE(usb_functions_adb),
                .functions      = usb_functions_adb,
        },
};

/* standard android USB platform data */
static struct android_usb_platform_data andusb_plat = {
        .vendor_id              = 0x0955,
        .product_id             = 0x7100,
        .manufacturer_name      = "Toshiba",
        .product_name           = "Folio 100",
        .serial_number          = NULL,
        .num_products = ARRAY_SIZE(usb_products),
        .products = usb_products,
        .num_functions = ARRAY_SIZE(usb_functions_adb),
        .functions = usb_functions_adb,
};

static struct platform_device androidusb_device = {
        .name   = "android_usb",
        .id     = -1,
        .dev    = {
                .platform_data  = &andusb_plat,
        },
};

struct tag_tegra {
	__u32 bootarg_key;
	__u32 bootarg_len;
	char bootarg[1];
};

static int __init parse_tag_nvidia(const struct tag *tag)
{
	return 0;
}
__tagtable(ATAG_NVIDIA, parse_tag_nvidia);

static struct tegra_utmip_config utmi_phy_config = {
	.hssync_start_delay = 0,
	.idle_wait_delay = 17,
	.elastic_limit = 16,
	.term_range_adj = 6,
	.xcvr_setup = 9,
	.xcvr_lsfslew = 2,
	.xcvr_lsrslew = 2,
};

static struct tegra_ehci_platform_data tegra_ehci_pdata = {
	.phy_config = &utmi_phy_config,
	.operating_mode = TEGRA_USB_HOST,
	.power_down_on_bus_suspend = 1,
};

static struct plat_serial8250_port debug_uart_platform_data[] = {
	{
		.membase	= IO_ADDRESS(TEGRA_UARTD_BASE),
		.mapbase	= TEGRA_UARTD_BASE,
		.irq		= INT_UARTD,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 216000000,
	}, {
		.flags		= 0
	}
};

static struct platform_device debug_uart = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = debug_uart_platform_data,
	},
};

/* PDA power */
static struct pda_power_pdata pda_power_pdata = {
};

static struct platform_device pda_power_device = {
	.name   = "pda_power",
	.id     = -1,
	.dev    = {
		.platform_data  = &pda_power_pdata,
	},
};

static struct tegra_i2c_platform_data betelgeuse_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
};

static const struct tegra_pingroup_config i2c2_ddc = {
	.pingroup	= TEGRA_PINGROUP_DDC,
	.func		= TEGRA_MUX_I2C2,
};

static const struct tegra_pingroup_config i2c2_gen2 = {
	.pingroup	= TEGRA_PINGROUP_PTA,
	.func		= TEGRA_MUX_I2C2,
};

static struct tegra_i2c_platform_data betelgeuse_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 2,
	.bus_clk_rate	= { 400000, 100000 },
	.bus_mux	= { &i2c2_ddc, &i2c2_gen2 },
	.bus_mux_len	= { 1, 1 },
};

static struct tegra_i2c_platform_data betelgeuse_i2c3_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
};

static struct tegra_i2c_platform_data betelgeuse_dvc_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.is_dvc		= true,
};

#define WM8903_GPn_FN_DMIC_LR_CLK_OUTPUT 6

static struct wm8903_platform_data wm8903_pdata = {
        .irq_active_low = 0,
        .micdet_cfg = 0,
        .micdet_delay = 100,
        .gpio_base = WM8903_GPIO_BASE,
        .gpio_cfg = {
		(WM8903_GPn_FN_DMIC_LR_CLK_OUTPUT << WM8903_GP1_FN_SHIFT),
		(WM8903_GPn_FN_DMIC_LR_CLK_OUTPUT << WM8903_GP2_FN_SHIFT)
		| WM8903_GP1_DIR_MASK,
                0,                     /* as output pin */
		WM8903_GPIO_NO_CONFIG,
		WM8903_GPIO_NO_CONFIG,
        },
};

	

static struct i2c_board_info __initdata wm8903_device = {
	I2C_BOARD_INFO("wm8903", 0x1a),
	.platform_data = &wm8903_pdata,
	.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_CDC_IRQ),
};

static struct i2c_board_info __initdata ak8975_device = {
	I2C_BOARD_INFO("ak8975", 0x0c),
	.irq            = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_MAGNETOMETER),
};

static struct tegra_audio_platform_data tegra_spdif_pdata = {
        .dma_on = true,  /* use dma by default */
        .spdif_clk_rate = 5644800,
};

/*
static struct tegra_audio_platform_data tegra_audio_pdata = {
	.i2s_master	= false,
	.dsp_master	= false,
	.dma_on		= true, */ /* use dma by default */
/*
	.i2s_clk_rate	= 240000000,
	.dap_clk	= "clk_dev1",
	.audio_sync_clk = "audio_2x",
	.mode		= I2S_BIT_FORMAT_I2S,
	.fifo_fmt	= I2S_FIFO_16_LSB,
	.bit_size	= I2S_BIT_SIZE_16,
};
*/

static struct tegra_audio_platform_data tegra_audio_pdata[] = {
        /* For I2S1 */
        [0] = {
                .i2s_master     = true,
                .dma_on         = true,  /* use dma by default */
                .i2s_master_clk = 44100,
                .i2s_clk_rate   = 11289600,
                .dap_clk        = "clk_dev1",
                .audio_sync_clk = "audio_2x",
                .mode           = I2S_BIT_FORMAT_I2S,
                .fifo_fmt       = I2S_FIFO_PACKED,
                .bit_size       = I2S_BIT_SIZE_16,
                .i2s_bus_width = 32,
                .dsp_bus_width = 16,
        },
        /* For I2S2 */
        [1] = {
                .i2s_master     = true,
                .dma_on         = true,  /* use dma by default */
                .i2s_master_clk = 8000,
                .dsp_master_clk = 8000,
                .i2s_clk_rate   = 2000000,
                .dap_clk        = "clk_dev1",
                .audio_sync_clk = "audio_2x",
                .mode           = I2S_BIT_FORMAT_DSP,
                .fifo_fmt       = I2S_FIFO_16_LSB,
                .bit_size       = I2S_BIT_SIZE_16,
                .i2s_bus_width = 32,
                .dsp_bus_width = 16,
        }
};

static struct tegra_das_platform_data tegra_das_pdata = {
        .dap_clk = "clk_dev1",
        .tegra_dap_port_info_table = {
                /* I2S1 <--> DAC1 <--> DAP1 <--> Hifi Codec */
                [0] = {
                        .dac_port = tegra_das_port_i2s1,
                        .dap_port = tegra_das_port_dap1,
                        .codec_type = tegra_audio_codec_type_hifi,
                        .device_property = {
                                .num_channels = 2,
                                .bits_per_sample = 16,
                                .rate = 44100,
                                .dac_dap_data_comm_format =
                                                dac_dap_data_format_all,
                        },
                },
                [1] = {
                        .dac_port = tegra_das_port_none,
                        .dap_port = tegra_das_port_none,
                        .codec_type = tegra_audio_codec_type_none,
                        .device_property = {
                                .num_channels = 0,
                                .bits_per_sample = 0,
                                .rate = 0,
                                .dac_dap_data_comm_format = 0,
                        },
                },
                [2] = {
                        .dac_port = tegra_das_port_none,
                        .dap_port = tegra_das_port_none,
                        .codec_type = tegra_audio_codec_type_none,
                        .device_property = {
                                .num_channels = 0,
                                .bits_per_sample = 0,
                                .rate = 0,
                                .dac_dap_data_comm_format = 0,
                        },
                },
                /* I2S2 <--> DAC2 <--> DAP4 <--> BT SCO Codec */
                [3] = {
                        .dac_port = tegra_das_port_i2s2,
                        .dap_port = tegra_das_port_dap4,
                        .codec_type = tegra_audio_codec_type_bluetooth,
                        .device_property = {
                                .num_channels = 1,
                                .bits_per_sample = 16,
                                .rate = 8000,
                                .dac_dap_data_comm_format =
                                        dac_dap_data_format_dsp,
                        },
                },
                [4] = {
                        .dac_port = tegra_das_port_none,
                        .dap_port = tegra_das_port_none,
                        .codec_type = tegra_audio_codec_type_none,
                        .device_property = {
                                .num_channels = 0,
                                .bits_per_sample = 0,
                                .rate = 0,
                                .dac_dap_data_comm_format = 0,
                        },
                },
        },

        .tegra_das_con_table = {
                [0] = {
                        .con_id = tegra_das_port_con_id_hifi,
                        .num_entries = 2,
                        .con_line = {
                                [0] = {tegra_das_port_i2s1, tegra_das_port_dap1, true},
                                [1] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
                        },
                },
                [1] = {
                        .con_id = tegra_das_port_con_id_bt_codec,
                        .num_entries = 4,
                        .con_line = {
                                [0] = {tegra_das_port_i2s2, tegra_das_port_dap4, true},
                                [1] = {tegra_das_port_dap4, tegra_das_port_i2s2, false},
                                [2] = {tegra_das_port_i2s1, tegra_das_port_dap1, true},
                                [3] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
                        },
                },
        }
};

static void betelgeuse_i2c_init(void)
{
	i2c_register_board_info(0, &wm8903_device, 1);
	i2c_register_board_info(4, &ak8975_device, 1);

	tegra_i2c_device1.dev.platform_data = &betelgeuse_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &betelgeuse_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &betelgeuse_i2c3_platform_data;
	tegra_i2c_device4.dev.platform_data = &betelgeuse_dvc_platform_data;

	platform_device_register(&tegra_i2c_device1);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device3);
	platform_device_register(&tegra_i2c_device4);
	
	//i2c_register_board_info(0, betelgeuse_i2c_bus1_board_info, ARRAY_SIZE(betelgeuse_i2c_bus1_board_info));
}

static struct platform_device *betelgeuse_devices[] __initdata = {
	&androidusb_device,
	&debug_uart,
	&pmu_device,
	&tegra_udc_device,
	&pda_power_device,
	&tegra_ehci3_device,
	&tegra_spi_device1,
	&tegra_spi_device2,
	&tegra_spi_device3,
	&tegra_spi_device4,
	&tegra_gart_device,
	&tegra_i2s_device1,
	&tegra_avp_device,
};

static void __init tegra_betelgeuse_fixup(struct machine_desc *desc,
	struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	//mi->bank[0].start = PHYS_OFFSET;
	//mi->bank[0].size = 448 * SZ_1M;
	mi->bank[0].size  = SHUTTLE_MEM_SIZE;
	mi->bank[0].size  = SHUTTLE_MEM_SIZE - SHUTTLE_GPU_MEM_SIZE;
	
	//mi->bank[1].start = SZ_512M;
	//mi->bank[1].size = SZ_512M;
}

static __initdata struct tegra_clk_init_table betelgeuse_clk_init_table[] = {
	/* name		parent		rate		enabled */
/*
	{ "clk_dev1",	NULL,		26000000,	true},
	{ "clk_m",	NULL,		12000000,	true},
	{ "3d",		"pll_m",	266400000,	true},
	{ "2d",		"pll_m",	266400000,	true},
	{ "vi",		"pll_m",	50000000,	true},
	{ "vi_sensor",	"pll_m",	111000000,	false},
	{ "epp",	"pll_m",	266400000,	true},
	{ "mpe",	"pll_m",	111000000,	false},
	{ "emc",	"pll_m",	666000000,	true},
	{ "pll_c",	"clk_m",	600000000,	true},
	{ "pll_c_out1",	"pll_c",	240000000,	true},
	{ "vde",	"pll_c",	240000000,	false},
	{ "pll_p",	"clk_m",	216000000,	true},
	{ "pll_p_out1",	"pll_p",	28800000,	true},
	{ "pll_a",	"pll_p_out1",	56448000,	true},
	{ "pll_a_out0",	"pll_a",	11289600,	true},
	{ "i2s1",	"pll_a_out0",	11289600,	true},
	{ "audio",	"pll_a_out0",	11289600,	true},
	{ "audio_2x",	"audio",	22579200,	false},
	{ "pll_p_out2",	"pll_p",	48000000,	true},
	{ "pll_p_out3",	"pll_p",	72000000,	true},
	{ "i2c1_i2c",	"pll_p_out3",	72000000,	true},
	{ "i2c2_i2c",	"pll_p_out3",	72000000,	true},
	{ "i2c3_i2c",	"pll_p_out3",	72000000,	true},
	{ "dvc_i2c",	"pll_p_out3",	72000000,	true},
	{ "csi",	"pll_p_out3",	72000000,	false},
	{ "pll_p_out4",	"pll_p",	108000000,	true},
	{ "sclk",	"pll_p_out4",	108000000,	true},
	{ "hclk",	"sclk",		108000000,	true},
	{ "pclk",	"hclk",		54000000,	true},
	{ "apbdma",	"hclk",		54000000,	true},
	{ "spdif_in",	"pll_p",	36000000,	false},
	{ "csite",	"pll_p",	144000000,	true},
	{ "uartd",	"pll_p",	216000000,	true},
	{ "host1x",	"pll_p",	144000000,	true},
	{ "disp1",	"pll_p",	216000000,	true},
	{ "pll_d",	"clk_m",	1000000,	false},
	{ "pll_d_out0",	"pll_d",	500000,		false},
	{ "dsi",	"pll_d",	1000000,	false},
	{ "pll_u",	"clk_m",	480000000,	true},
	{ "clk_d",	"clk_m",	24000000,	true},
	{ "timer",	"clk_m",	12000000,	true},
	{ "i2s2",	"clk_m",	11289600,	false},
	{ "spdif_out",	"clk_m",	12000000,	false},
	{ "spi",	"clk_m",	12000000,	false},
	{ "xio",	"clk_m",	12000000,	false},
	{ "twc",	"clk_m",	12000000,	false},
	{ "sbc1",	"clk_m",	12000000,	false},
	{ "sbc2",	"clk_m",	12000000,	false},
	{ "sbc3",	"clk_m",	12000000,	false},
	{ "sbc4",	"clk_m",	12000000,	false},
	{ "ide",	"clk_m",	12000000,	false},
	{ "ndflash",	"clk_m",	108000000,	true},
	{ "vfir",	"clk_m",	12000000,	false},
	{ "sdmmc1",	"clk_m",	48000000,	true},
	{ "sdmmc2",	"clk_m",	48000000,	true},
	{ "sdmmc3",	"clk_m",	48000000,	false},
	{ "sdmmc4",	"clk_m",	48000000,	true},
	{ "la",		"clk_m",	12000000,	false},
	{ "owr",	"clk_m",	12000000,	false},
	{ "nor",	"clk_m",	12000000,	false},
	{ "mipi",	"clk_m",	12000000,	false},
	{ "i2c1",	"clk_m",	3000000,	false},
	{ "i2c2",	"clk_m",	3000000,	false},
	{ "i2c3",	"clk_m",	3000000,	false},
	{ "dvc",	"clk_m",	3000000,	false},
	{ "uarta",	"clk_m",	12000000,	false},
	{ "uartb",	"clk_m",	12000000,	false},
	{ "uartc",	"clk_m",	12000000,	false},
	{ "uarte",	"clk_m",	12000000,	false},
	{ "cve",	"clk_m",	12000000,	false},
	{ "tvo",	"clk_m",	12000000,	false},
	{ "hdmi",	"clk_m",	12000000,	false},
	{ "tvdac",	"clk_m",	12000000,	false},
	{ "disp2",	"clk_m",	12000000,	false},
	{ "usbd",	"clk_m",	12000000,	false},
	{ "usb2",	"clk_m",	12000000,	false},
	{ "usb3",	"clk_m",	12000000,	true},
	{ "isp",	"clk_m",	12000000,	false},
	{ "csus",	"clk_m",	12000000,	false},
	{ "pwm",	"clk_32k",	32768,		false},
	{ "clk_32k",	NULL,		32768,		true},
	{ "pll_s",	"clk_32k",	32768,		false},
	{ "rtc",	"clk_32k",	32768,		true},
	{ "kbc",	"clk_32k",	32768,		true},
	{ NULL,		NULL,		0,		0},
*/

	{ "clk_32k",    NULL,           32768,          true}, // always on
	{ "clk_m",      NULL,           0,		true}, // must be always on - Frequency will be autodetected
	{ "pll_s",      "clk_32k",      12000000,       true},

        { "apbdma",     "hclk",         54000000,       true},
        { "audio",      "pll_a_out0",   11289600,       true},
        { "audio_2x",   "audio",        22579200,       false},
	{ "spdif_out",  "pll_a_out0",   5644800,        false},
        { "uarta",      "clk_m",        12000000,       true},
        { "uartd",      "pll_p",        216000000,      true},
        { "pwm",        "clk_32k",      32768,          true},
        { "clk_d",      "clk_m",        24000000,       true},
        { "pll_a",      "pll_p_out1",   56448000,       true},
        { "pll_a_out0", "pll_a",        11289600,       true},
        { "i2s1",       "pll_a_out0",   11289600,       false},
        { "i2s2",       "pll_a_out0",   11289600,       false},

	// pll_c is used as graphics clock and system clock
        { "pll_c",      "clk_m",        600000000,      true},
        { "pll_c_out1", "pll_c",        240000000,      true},

	// pll_p is used as system clock - and for ulpi
        { "pll_p",      "clk_m",        216000000,      true},          // must be always on
	{ "pll_p_out1", "pll_p",        28800000,       true},          // must be always on - audio clocks ...
	{ "pll_p_out2", "pll_p",        108000000,      true},  // must be always on
	{ "pll_p_out3", "pll_p",        72000000,       true},  // must be always on - i2c, camera
	{ "pll_p_out4", "pll_p",        26000000,       true},  // must be always on - USB ulpi
        //{ "pll_p_out4", "pll_p",        24000000,       true},
	//

	// pll_m is used as memory clock - bootloader also uses it this way
	{ "pll_m",      "clk_m",        666000000,      true},          // always on - memory clocks
	{ "pll_m_out1", "pll_m",        222000000,      true},          // always on - unused ?
	{ "emc",        "pll_m",        333000000,      true},          // always on


        { "i2c1_i2c",   "pll_p_out3",   72000000,       true},
        { "i2c2_i2c",   "pll_p_out3",   72000000,       true},
        { "i2c3_i2c",   "pll_p_out3",   72000000,       true},
        { "dvc_i2c",    "pll_p_out3",   72000000,       true},
        { "i2c1",       "clk_m",        3000000,        false},
        { "i2c2",       "clk_m",        3000000,        false},
        { "i2c3",       "clk_m",        3000000,        false},
	{ "sclk",       "pll_p_out2",   108000000,      true},          // must be always on
	{ "avp.sclk",   "sclk",		108000000,      false},
	{ "vcp",        "clk_m",        12000000,       false},
        { "bsea",       "clk_m",        12000000,       false},
	{ "vde",        "pll_p",        28800000,       false},
	{ "kbc",        "clk_32k",      32768,          false},         // tegra-kbc
        { NULL,         NULL,           0,              0},
};

static struct tegra_suspend_platform_data betelgeuse_suspend = {
	.cpu_timer = 5000,
	.cpu_off_timer = 5000,
	.core_timer = 0x7e7e,
	.core_off_timer = 0x7f,
	.separate_req = true,
	.corereq_high = false,
	.sysclkreq_high = true,
	.suspend_mode = TEGRA_SUSPEND_LP0,
};

static int betelgeuse_ehci_init(void)
{
        int gpio_status;

        gpio_status = gpio_request(TEGRA_GPIO_USB1, "VBUS_USB1");
        if (gpio_status < 0) {
                pr_err("VBUS_USB1 request GPIO FAILED\n");
                WARN_ON(1);
        }
        tegra_gpio_enable(TEGRA_GPIO_USB1);
        gpio_status = gpio_direction_output(TEGRA_GPIO_USB1, 1);
        if (gpio_status < 0) {
                pr_err("VBUS_USB1 request GPIO DIRECTION FAILED\n");
                WARN_ON(1);
        }
        gpio_set_value(TEGRA_GPIO_USB1, 1);
	
	tegra_ehci3_device.dev.platform_data = &tegra_ehci_pdata;
	/*
        tegra_ehci1_device.dev.platform_data = &tegra_ehci_pdata[0];
        tegra_ehci2_device.dev.platform_data = &tegra_ehci_pdata[1];
        tegra_ehci3_device.dev.platform_data = &tegra_ehci_pdata[2];

        platform_device_register(&tegra_ehci1_device);
        platform_device_register(&tegra_ehci2_device);
        platform_device_register(&tegra_ehci3_device);
	*/

        return 0;
}

static void __init tegra_betelgeuse_init(void)
{
	tegra_common_init();
	betelgeuse_emc_init();

	tegra_init_suspend(&betelgeuse_suspend);

	tegra_clk_init_from_table(betelgeuse_clk_init_table);

	betelgeuse_pinmux_init();

	betelgeuse_ehci_init();

	//tegra_i2s_device1.dev.platform_data = &tegra_audio_pdata;
	tegra_i2s_device1.dev.platform_data = &tegra_audio_pdata[0];
        tegra_i2s_device2.dev.platform_data = &tegra_audio_pdata[1];
        tegra_spdif_device.dev.platform_data = &tegra_spdif_pdata;
	tegra_das_device.dev.platform_data = &tegra_das_pdata;

	platform_add_devices(betelgeuse_devices, ARRAY_SIZE(betelgeuse_devices));

	betelgeuse_panel_init();
	betelgeuse_sdhci_init();
	betelgeuse_i2c_init();
	betelgeuse_power_init();
}

MACHINE_START(HARMONY, "harmony")
	.boot_params  = 0x00000100,
	.phys_io        = IO_APB_PHYS,
	.io_pg_offst    = ((IO_APB_VIRT) >> 18) & 0xfffc,
	.fixup		= tegra_betelgeuse_fixup,
	.init_irq       = tegra_init_irq,
	.init_machine   = tegra_betelgeuse_init,
	.map_io         = tegra_map_common_io,
	.timer          = &tegra_timer,
MACHINE_END

MACHINE_START(TEGRA_LEGACY, "tegra_legacy")
	.boot_params  = 0x00000100,
	.phys_io        = IO_APB_PHYS,
	.io_pg_offst    = ((IO_APB_VIRT) >> 18) & 0xfffc,
	.fixup          = tegra_betelgeuse_fixup,
	.init_irq       = tegra_init_irq,
	.init_machine   = tegra_betelgeuse_init,
	.map_io         = tegra_map_common_io,
	.timer          = &tegra_timer,
MACHINE_END
