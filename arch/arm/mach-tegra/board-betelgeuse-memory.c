/*
 * arch/arm/mach-tegra/board-betelgeuse-memory.c
 *
 * Copyright (c) 2011 Artem Makhutov
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
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>

#include <asm/mach-types.h>

#include <mach/iomap.h>

#include "board-betelgeuse.h"
#include "fuse.h"
#include "tegra2_emc.h"
#include "board.h"

static const struct tegra_emc_table betelgeuse_emc_tables_hynix_333Mhz[] = {
        {
                .rate = 166500,   /* SDRAM frequency */
                .regs = {
                        0x0000000a,   /* RC */
                        0x00000021,   /* RFC */
                        0x00000008,   /* RAS */
                        0x00000003,   /* RP */
                        0x00000004,   /* R2W */
                        0x00000004,   /* W2R */
                        0x00000002,   /* R2P */
                        0x0000000c,   /* W2P */
                        0x00000003,   /* RD_RCD */
                        0x00000003,   /* WR_RCD */
                        0x00000002,   /* RRD */
                        0x00000001,   /* REXT */
                        0x00000004,   /* WDV */
                        0x00000005,   /* QUSE */
                        0x00000004,   /* QRST */
                        0x00000009,   /* QSAFE */
                        0x0000000d,   /* RDV */
                        0x000004df,   /* REFRESH */
                        0x00000000,   /* BURST_REFRESH_NUM */
                        0x00000003,   /* PDEX2WR */
                        0x00000003,   /* PDEX2RD */
                        0x00000003,   /* PCHG2PDEN */
                        0x00000003,   /* ACT2PDEN */
                        0x00000001,   /* AR2PDEN */
                        0x0000000a,   /* RW2PDEN */
                        0x000000c8,   /* TXSR */
                        0x00000003,   /* TCKE */
                        0x00000006,   /* TFAW */
                        0x00000004,   /* TRPAB */
                        0x0000000f,   /* TCLKSTABLE */
                        0x00000002,   /* TCLKSTOP */
                        0x00000000,   /* TREFBW */
                        0x00000000,   /* QUSE_EXTRA */
                        0x00000002,   /* FBIO_CFG6 */
                        0x00000000,   /* ODT_WRITE */
                        0x00000000,   /* ODT_READ */
                        0x00000083,   /* FBIO_CFG5 */
                        0xa04004ae,   /* CFG_DIG_DLL */
                        0x007fd010,   /* DLL_XFORM_DQS */
                        0x00000000,   /* DLL_XFORM_QUSE */
                        0x00000000,   /* ZCAL_REF_CNT */
                        0x00000000,   /* ZCAL_WAIT_CNT */
                        0x00000000,   /* AUTO_CAL_INTERVAL */
                        0x00000000,   /* CFG_CLKTRIM_0 */
                        0x00000000,   /* CFG_CLKTRIM_1 */
                        0x00000000,   /* CFG_CLKTRIM_2 */
                }
        }, {
                .rate = 333000,   /* SDRAM frequency */
                .regs = {
                        0x00000014,   /* RC */
                        0x00000041,   /* RFC */
                        0x0000000f,   /* RAS */
                        0x00000005,   /* RP */
                        0x00000004,   /* R2W */
                        0x00000005,   /* W2R */
                        0x00000003,   /* R2P */
                        0x0000000c,   /* W2P */
                        0x00000005,   /* RD_RCD */
                        0x00000005,   /* WR_RCD */
                        0x00000003,   /* RRD */
                        0x00000001,   /* REXT */
                        0x00000004,   /* WDV */
                        0x00000005,   /* QUSE */
                        0x00000004,   /* QRST */
                        0x00000009,   /* QSAFE */
                        0x0000000d,   /* RDV */
                        0x000009ff,   /* REFRESH */
                        0x00000000,   /* BURST_REFRESH_NUM */
                        0x00000003,   /* PDEX2WR */
                        0x00000003,   /* PDEX2RD */
                        0x00000005,   /* PCHG2PDEN */
                        0x00000005,   /* ACT2PDEN */
                        0x00000001,   /* AR2PDEN */
                        0x0000000f,   /* RW2PDEN */
                        0x000000c8,   /* TXSR */
                        0x00000003,   /* TCKE */
                        0x0000000c,   /* TFAW */
                        0x00000006,   /* TRPAB */
                        0x0000000f,   /* TCLKSTABLE */
                        0x00000002,   /* TCLKSTOP */
                        0x00000000,   /* TREFBW */
                        0x00000000,   /* QUSE_EXTRA */
                        0x00000002,   /* FBIO_CFG6 */
                        0x00000000,   /* ODT_WRITE */
                        0x00000000,   /* ODT_READ */
                        0x00000083,   /* FBIO_CFG5 */
                        0xe034048b,   /* CFG_DIG_DLL */
                        0x007e8010,   /* DLL_XFORM_DQS */
                        0x00000000,   /* DLL_XFORM_QUSE */
                        0x00000000,   /* ZCAL_REF_CNT */
                        0x00000000,   /* ZCAL_WAIT_CNT */
                        0x00000000,   /* AUTO_CAL_INTERVAL */
                        0x00000000,   /* CFG_CLKTRIM_0 */
                        0x00000000,   /* CFG_CLKTRIM_1 */
                        0x00000000,   /* CFG_CLKTRIM_2 */
                }
        }
};

static const struct tegra_emc_table betelgeuse_emc_tables_hynix_400Mhz[] = {
	{
		.rate = 190000,   /* SDRAM frequency */
		.regs = {
			0x0000000c,   /* RC */
			0x00000026,   /* RFC */
			0x00000009,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x0000059f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000b,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000007,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06204ae,   /* CFG_DIG_DLL */
			0x007dc010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 380000,   /* SDRAM frequency */
		.regs = {
			0x00000017,   /* RC */
			0x0000004b,   /* RFC */
			0x00000012,   /* RAS */
			0x00000006,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000006,   /* RD_RCD */
			0x00000006,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x00000b5f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000006,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x00000011,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000e,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe044048b,   /* CFG_DIG_DLL */
			0x007d8010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

static const struct tegra_emc_chip betelgeuse_t20_emc_chips[] = {
        {
                .description = "Hynix 333MHz",
                .mem_manufacturer_id = -1,
                .mem_revision_id1 = -1,
                .mem_revision_id2 = -1,
                .mem_pid = -1,
                .table = betelgeuse_emc_tables_hynix_333Mhz,
                .table_size = ARRAY_SIZE(betelgeuse_emc_tables_hynix_333Mhz)
        },
};

static const struct tegra_emc_chip betelgeuse_t25_emc_chips[] = {
        {
                .description = "Hynix 400MHz",
                .mem_manufacturer_id = -1,
                .mem_revision_id1 = -1,
                .mem_revision_id2 = -1,
                .mem_pid = -1,
                .table = betelgeuse_emc_tables_hynix_400Mhz,
                .table_size = ARRAY_SIZE(betelgeuse_emc_tables_hynix_400Mhz)
        },
};

static const struct tegra_emc_chip betelgeuse_siblings_emc_chips[] = {
};

#define TEGRA20_SKU		0x0008
#define TEGRA25_SKU             0x0B00

void __init betelgeuse_emc_init(void)
{
	struct board_info BoardInfo;
	tegra_get_board_info(&BoardInfo);
	pr_info("EMC table: tegra_sku_id %d\n", BoardInfo.sku);

	if (BoardInfo.sku == TEGRA20_SKU) {
		pr_info("betelgeuse_emc_init: using betelgeuse_t20_emc_chips\n");
//		tegra_init_emc(betelgeuse_t20_emc_chips,
//			ARRAY_SIZE(betelgeuse_t20_emc_chips));
	} else if (BoardInfo.sku == TEGRA25_SKU) {
		pr_info("betelgeuse_emc_init: using betelgeuse_t25_emc_chips\n");
		tegra_init_emc(betelgeuse_t25_emc_chips,
			ARRAY_SIZE(betelgeuse_t25_emc_chips));
	} else {
		pr_info("betelgeuse_emc_init: using betelgeuse_siblings_emc_chips\n");
		tegra_init_emc(betelgeuse_siblings_emc_chips,
					ARRAY_SIZE(betelgeuse_siblings_emc_chips));
	}
}
