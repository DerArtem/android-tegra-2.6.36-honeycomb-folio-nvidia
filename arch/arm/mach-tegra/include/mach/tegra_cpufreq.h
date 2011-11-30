/*
 * arch/arm/mach-tegra/include/mach/tegra_cpufreq.h
 *
 * Copyright (c) 2011, NVIDIA Corporation.
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

#ifndef __MACH_TEGRA_CPUFREQ_H
#define __MACH_TEGRA_CPUFREQ_H

#if defined CONFIG_HAS_EARLYSUSPEND && defined CONFIG_CPU_FREQ
void cpufreq_save_default_governor(void);
void cpufreq_restore_default_governor(void);
void cpufreq_set_conservative_governor(void);
#endif

#endif
