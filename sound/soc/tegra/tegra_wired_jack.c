/*
 * sound/soc/tegra/tegra_wired_jack.c
 *
 * Copyright (c) 2011, NVIDIA Corporation.
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

#include <linux/types.h>
#include <linux/gpio.h>
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif
#include <linux/notifier.h>
#include <sound/jack.h>
#include <sound/soc.h>
#include <mach/audio.h>

#include "tegra_soc.h"

#define HEAD_DET_GPIO 0
#define MIC_DET_GPIO  1
#define SPK_EN_GPIO   3

struct wired_jack_conf tegra_wired_jack_conf = {
	-1, -1, -1, -1, 0, NULL, NULL
};

/* Based on hp_gpio and mic_gpio, hp_gpio is active low */
enum {
	HEADSET_WITHOUT_MIC = 0x00,
	HEADSET_WITH_MIC = 0x01,
	NO_DEVICE = 0x10,
	MIC = 0x11,
};

/* These values are copied from WiredAccessoryObserver */
enum headset_state {
	BIT_NO_HEADSET = 0,
	BIT_HEADSET = (1 << 0),
	BIT_HEADSET_NO_MIC = (1 << 1),
};

/* jack */
static struct snd_soc_jack *tegra_wired_jack;

static struct snd_soc_jack_gpio wired_jack_gpios[] = {
	{
		/* gpio pin depends on board traits */
		.name = "headphone-detect-gpio",
		.report = SND_JACK_HEADPHONE,
		.invert = 1,
		.debounce_time = 200,
	},
	{
		/* gpio pin depens on board traits */
		.name = "mic-detect-gpio",
		.report = SND_JACK_MICROPHONE,
		.invert = 1,
		.debounce_time = 200,
	},
};

#ifdef CONFIG_SWITCH
static struct switch_dev wired_switch_dev = {
	.name = "h2w",
};

void tegra_switch_set_state(int state)
{
	switch_set_state(&wired_switch_dev, state);
}

static enum headset_state get_headset_state(void)
{
	enum headset_state state = BIT_NO_HEADSET;
	int flag = 0;
	int hp_gpio = -1;
	int mic_gpio = -1;

	/* hp_det_n is low active pin */
	if (tegra_wired_jack_conf.hp_det_n != -1)
		hp_gpio = gpio_get_value(tegra_wired_jack_conf.hp_det_n);
	if (tegra_wired_jack_conf.cdc_irq != -1)
		mic_gpio = gpio_get_value(tegra_wired_jack_conf.cdc_irq);

	pr_debug("hp_gpio:%d, mic_gpio:%d\n", hp_gpio, mic_gpio);

	flag = (hp_gpio << 4) | mic_gpio;

	switch (flag) {
	case NO_DEVICE:
		state = BIT_NO_HEADSET;
		break;
	case HEADSET_WITH_MIC:
		state = BIT_HEADSET;
		break;
	case MIC:
		/* mic: would not report */
		break;
	case HEADSET_WITHOUT_MIC:
		state = BIT_HEADSET_NO_MIC;
		break;
	default:
		state = BIT_NO_HEADSET;
	}

	return state;
}

static int wired_switch_notify(struct notifier_block *self,
			      unsigned long action, void* dev)
{
	tegra_switch_set_state(get_headset_state());

	return NOTIFY_OK;
}


void tegra_jack_resume(void)
{
	tegra_switch_set_state(get_headset_state());
}

static struct notifier_block wired_switch_nb = {
	.notifier_call = wired_switch_notify,
};
#endif

/* platform driver */
static int tegra_wired_jack_probe(struct platform_device *pdev)
{
	int ret;
	int hp_det_n, cdc_irq;
	int en_mic_int, en_mic_ext;
	int en_spkr;
	struct tegra_wired_jack_conf *pdata;

	pdata = (struct tegra_wired_jack_conf *)pdev->dev.platform_data;
	if (!pdata || !pdata->hp_det_n || !pdata->en_spkr ||
	    !pdata->cdc_irq || !pdata->en_mic_int || !pdata->en_mic_ext) {
		pr_err("Please set up gpio pins for jack.\n");
		return -EBUSY;
	}

	hp_det_n = pdata->hp_det_n;
	wired_jack_gpios[HEAD_DET_GPIO].gpio = hp_det_n;

	cdc_irq = pdata->cdc_irq;
	wired_jack_gpios[MIC_DET_GPIO].gpio = cdc_irq;

	ret = snd_soc_jack_add_gpios(tegra_wired_jack,
				     ARRAY_SIZE(wired_jack_gpios),
				     wired_jack_gpios);
	if (ret) {
		pr_err("Could NOT set up gpio pins for jack.\n");
		snd_soc_jack_free_gpios(tegra_wired_jack,
					ARRAY_SIZE(wired_jack_gpios),
					wired_jack_gpios);
		return ret;
	}

	/* Mic switch controlling pins */
	en_mic_int = pdata->en_mic_int;
	en_mic_ext = pdata->en_mic_ext;

	ret = gpio_request(en_mic_int, "en_mic_int");
	if (ret) {
		pr_err("Could NOT get gpio for internal mic controlling.\n");
		gpio_free(en_mic_int);
	}
	gpio_direction_output(en_mic_int, 0);
	gpio_export(en_mic_int, false);

	ret = gpio_request(en_mic_ext, "en_mic_ext");
	if (ret) {
		pr_err("Could NOT get gpio for external mic controlling.\n");
		gpio_free(en_mic_ext);
	}
	gpio_direction_output(en_mic_ext, 0);
	gpio_export(en_mic_ext, false);

	en_spkr = pdata->en_spkr;
	ret = gpio_request(en_spkr, "en_spkr");
	if (ret) {
		pr_err("Could NOT set up gpio pin for amplifier.\n");
		gpio_free(en_spkr);
	}
	gpio_direction_output(en_spkr, 0);
	gpio_export(en_spkr, false);

	if (pdata->spkr_amp_reg)
		tegra_wired_jack_conf.amp_reg =
			regulator_get(NULL, pdata->spkr_amp_reg);
	tegra_wired_jack_conf.amp_reg_enabled = 0;

	/* restore configuration of these pins */
	tegra_wired_jack_conf.hp_det_n = hp_det_n;
	tegra_wired_jack_conf.en_mic_int = en_mic_int;
	tegra_wired_jack_conf.en_mic_ext = en_mic_ext;
	tegra_wired_jack_conf.cdc_irq = cdc_irq;
	tegra_wired_jack_conf.en_spkr = en_spkr;

	// Communicate the jack connection state at device bootup
	tegra_switch_set_state(get_headset_state());

#ifdef CONFIG_SWITCH
	snd_soc_jack_notifier_register(tegra_wired_jack,
				       &wired_switch_nb);
#endif
	return ret;
}

static int tegra_wired_jack_remove(struct platform_device *pdev)
{
	snd_soc_jack_free_gpios(tegra_wired_jack,
				ARRAY_SIZE(wired_jack_gpios),
				wired_jack_gpios);

	gpio_free(tegra_wired_jack_conf.en_mic_int);
	gpio_free(tegra_wired_jack_conf.en_mic_ext);
	gpio_free(tegra_wired_jack_conf.en_spkr);

	if (tegra_wired_jack_conf.amp_reg) {
		if (tegra_wired_jack_conf.amp_reg_enabled)
			regulator_disable(tegra_wired_jack_conf.amp_reg);
		regulator_put(tegra_wired_jack_conf.amp_reg);
	}

	return 0;
}

static struct platform_driver tegra_wired_jack_driver = {
	.probe = tegra_wired_jack_probe,
	.remove = tegra_wired_jack_remove,
	.driver = {
		.name = "tegra_wired_jack",
		.owner = THIS_MODULE,
	},
};


int tegra_jack_init(struct snd_soc_codec *codec)
{
	int ret;

	if (!codec)
		return -1;

	tegra_wired_jack = kzalloc(sizeof(*tegra_wired_jack), GFP_KERNEL);
	if (!tegra_wired_jack) {
		pr_err("failed to allocate tegra_wired_jack \n");
		return -ENOMEM;
	}

	/* Add jack detection */
	ret = snd_soc_jack_new(codec->socdev->card, "Wired Accessory Jack",
			       SND_JACK_HEADSET, tegra_wired_jack);
	if (ret < 0)
		goto failed;

#ifdef CONFIG_SWITCH
	/* Addd h2w swith class support */
	ret = switch_dev_register(&wired_switch_dev);
	if (ret < 0)
		goto switch_dev_failed;
#endif

	ret = platform_driver_register(&tegra_wired_jack_driver);
	if (ret < 0)
		goto platform_dev_failed;

	return 0;

#ifdef CONFIG_SWITCH
switch_dev_failed:
	switch_dev_unregister(&wired_switch_dev);
#endif
platform_dev_failed:
	platform_driver_unregister(&tegra_wired_jack_driver);
failed:
	if (tegra_wired_jack) {
		kfree(tegra_wired_jack);
		tegra_wired_jack = 0;
	}
	return ret;
}

void tegra_jack_exit(void)
{
#ifdef CONFIG_SWITCH
	switch_dev_unregister(&wired_switch_dev);
#endif
	platform_driver_unregister(&tegra_wired_jack_driver);

	if (tegra_wired_jack) {
		kfree(tegra_wired_jack);
		tegra_wired_jack = 0;
	}
}
