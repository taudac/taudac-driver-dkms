/*
 * ASoC Driver for TauDAC
 *
 * Author:	Sergej Sawazki <taudac@gmx.de>
 *		Copyright 2015
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "../codecs/wm8741.h"

#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/clk.h>

/*
 * clocks
 */
enum {
	BCLK_CPU,
	BCLK_DACL,
	BCLK_DACR,
	NUM_BCLKS
};

enum {
	LRCLK_CPU,
	LRCLK_DACL,
	LRCLK_DACR,
	NUM_LRCLKS
};

struct snd_soc_card_drvdata {
	struct clk *mclk24;
	struct clk *mclk22;
	struct clk *mclk_mux;
	struct clk *mclk_gate;
	struct clk *bclk[NUM_BCLKS];
	struct clk *lrclk[NUM_LRCLKS];
	bool mclk_enabled;
	bool bclk_prepared[NUM_BCLKS];
	bool lrclk_prepared[NUM_BCLKS];
};

static int taudac_i2s_clks_init(struct snd_soc_card_drvdata *drvdata)
{
	int ret, i, k, num_clks;
	struct clk *clkin, *pll, *ms;
	unsigned long clkin_rate, pll_rate, ms_rate;
	const int pll_clkin_ratio = 31;
	const int clkin_ms_ratio = 8;
	struct clk **clks;

	for (i = 0; i < 2; i++) {
		if (i == 0) {
			num_clks = NUM_BCLKS;
			clks = drvdata->bclk;
		} else {
			num_clks = NUM_LRCLKS;
			clks = drvdata->lrclk;
		}
		for (k = 0; k < num_clks; k++) {
			ms = clk_get_parent(clks[k]);
			if (IS_ERR(ms))
				return -EINVAL;

			pll = clk_get_parent(ms);
			if (IS_ERR(pll))
				return -EINVAL;

			clkin = clk_get_parent(pll);
			if (IS_ERR(clkin))
				return -EINVAL;

			clkin_rate = clk_get_rate(clkin);
			pll_rate = clkin_rate * pll_clkin_ratio;
			ms_rate = clkin_rate / clkin_ms_ratio;

			ret = clk_set_rate(pll, pll_rate);
			if (ret < 0)
				return ret;

			ret = clk_set_rate(ms, ms_rate);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static void taudac_i2s_clks_disable(struct snd_soc_card_drvdata *drvdata)
{
	int i;

	for (i = 0; i < NUM_BCLKS; i++) {
		if (drvdata->bclk_prepared[i]) {
			clk_disable_unprepare(drvdata->bclk[i]);
			drvdata->bclk_prepared[i] = false;
		}
	}

	for (i = 0; i < NUM_LRCLKS; i++) {
		if (drvdata->lrclk_prepared[i]) {
			clk_disable_unprepare(drvdata->lrclk[i]);
			drvdata->lrclk_prepared[i] = false;
		}
	}
}

static int taudac_i2s_clks_enable(struct snd_soc_card_drvdata *drvdata)
{
	int ret, i;

	for (i = 0; i < NUM_BCLKS; i++) {
		if (!drvdata->bclk_prepared[i]) {
			ret = clk_prepare_enable(drvdata->bclk[i]);
			if (ret != 0)
				return ret;
			drvdata->bclk_prepared[i] = true;
		}
	}

	for (i = 0; i < NUM_LRCLKS; i++) {
		if (!drvdata->lrclk_prepared[i]) {
			ret = clk_prepare_enable(drvdata->lrclk[i]);
			if (ret != 0)
				return ret;
			drvdata->lrclk_prepared[i] = true;
		}
	}

	return 0;
}

static void taudac_mclk_disable(struct snd_soc_card_drvdata *drvdata)
{
	if (drvdata->mclk_enabled) {
		clk_disable_unprepare(drvdata->mclk_gate);
		drvdata->mclk_enabled = false;
	}
}

static int taudac_mclk_enable(struct snd_soc_card_drvdata *drvdata,
		unsigned long mclk_rate)
{
	int ret;

	switch (mclk_rate) {
	case 22579200:
		ret = clk_set_parent(drvdata->mclk_mux, drvdata->mclk22);
		break;
	case 24576000:
		ret = clk_set_parent(drvdata->mclk_mux, drvdata->mclk24);
		break;
	default:
		return -EINVAL;
	}
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(drvdata->mclk_gate);
	if (ret < 0)
		return ret;

	drvdata->mclk_enabled = true;
	msleep(20);

	return 0;
}

static int taudac_i2s_clks_set_rate(struct snd_soc_card_drvdata *drvdata,
		unsigned long bclk_rate, unsigned long lrclk_rate)
{
	int ret, i;

	for (i = 0; i < NUM_BCLKS; i++) {
		ret = clk_set_rate(drvdata->bclk[i], bclk_rate);
		if (ret < 0)
			return ret;
	}

	for (i = 0; i < NUM_LRCLKS; i++) {
		ret = clk_set_rate(drvdata->lrclk[i], lrclk_rate);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static const struct reg_default wm8741_reg_updates[] = {
	/**
	 * R0..R3 - Attenuation:
	 *   set attenuation to 0dB and update the value on MSB write
	 */
	{0x00, 0x0000},
	{0x01, 0x0020},
	{0x02, 0x0000},
	{0x03, 0x0020},
	/**
	 * R4 - Volume Control:
	 *   enable Zero Detect, Mute and Volume Ramp, disable Zero Flag output
	 */
	{0x04, 0x0079},
	/**
	 * R5 - Format Control:
	 *   go to Power Down Mode, configure Normal Phase
	 * NOTE: In differential mono mode, the analogue output phase must
	 * remain set as 'Normal'.
	 */
	{0x05, 0x0080},
};

static int taudac_codecs_init(struct snd_soc_pcm_runtime *rtd)
{
	int ret, i, k;
	int num_codecs = rtd->num_codecs;
	struct snd_soc_dai **codec_dais = rtd->codec_dais;

	for (i = 0; i < num_codecs; i++) {
		/* change some codec settings */
		for (k = 0; k < ARRAY_SIZE(wm8741_reg_updates); k++) {
			ret = snd_soc_component_write(codec_dais[i]->component,
					wm8741_reg_updates[k].reg,
					wm8741_reg_updates[k].def);

			if (ret < 0) {
				dev_err(rtd->card->dev,
						"Failed to configure codecs: %d\n",
						ret);
				return ret;
			}
		}
	}

	return 0;
}

static void taudac_codecs_shutdown(struct snd_soc_pcm_runtime *rtd)
{
	int i;
	int num_codecs = rtd->num_codecs;
	struct snd_soc_dai **codec_dais = rtd->codec_dais;

	for (i = 0; i < num_codecs; i++) {
		/* disable codecs - avoid audible glitches */
		snd_soc_component_update_bits(codec_dais[i]->component,
				WM8741_FORMAT_CONTROL, WM8741_PWDN_MASK,
				WM8741_PWDN);
		/* clear codec sysclk - restore rate constrants */
		snd_soc_dai_set_sysclk(codec_dais[i], WM8741_SYSCLK, 0,
				SND_SOC_CLOCK_IN);
	}
}

static int taudac_codecs_prepare(struct snd_soc_pcm_runtime *rtd,
		unsigned int mclk_rate, unsigned int fmt)
{
	int ret, i;
	struct snd_soc_dai **codec_dais = rtd->codec_dais;
	int num_codecs = rtd->num_codecs;

	for (i = 0; i < num_codecs; i++) {
		/* set codec sysclk */
		ret = snd_soc_dai_set_sysclk(codec_dais[i],
				WM8741_SYSCLK, mclk_rate, SND_SOC_CLOCK_IN);
		if (ret < 0)
			return ret;

		/* set codec DAI configuration */
		ret = snd_soc_dai_set_fmt(codec_dais[i], fmt);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int taudac_codecs_startup(struct snd_soc_pcm_runtime *rtd)
{
	int ret, i;
	struct snd_soc_dai **codec_dais = rtd->codec_dais;
	int num_codecs = rtd->num_codecs;

	for (i = 0; i < num_codecs; i++) {
		ret = snd_soc_component_update_bits(codec_dais[i]->component,
				WM8741_FORMAT_CONTROL, WM8741_PWDN_MASK, 0);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
 * asoc controls
 */
static int codec_get_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_pcm_runtime *rtd = list_first_entry(
			&card->rtd_list, struct snd_soc_pcm_runtime, list);
	struct snd_soc_dai **codec_dais = rtd->codec_dais;
	int num_codecs = rtd->num_codecs;

	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int reg_val[2], val[2], item;
	int i;

	for (i = 0; i < num_codecs; i++) {
		reg_val[i] = snd_soc_component_read(codec_dai->component, e->reg);
		val[i] = (reg_val[i] >> e->shift_l) & e->mask;
		dev_dbg(codec_dais[i]->component->dev,
			"%s: reg = %u, reg_val = 0x%04x, val = 0x%04x",
			__func__, e->reg, reg_val[i], val[i]);
	}

	/* Both codecs should have the same value */
	if (WARN_ON(val[0] != val[1]))
		return -EINVAL;

	item = snd_soc_enum_val_to_item(e, val[0]);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int codec_put_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_pcm_runtime *rtd = list_first_entry(
			&card->rtd_list, struct snd_soc_pcm_runtime, list);
	struct snd_soc_dai **codec_dais = rtd->codec_dais;
	int num_codecs = rtd->num_codecs;

	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	unsigned int val;
	unsigned int mask;
	int ret, i;

	if (item[0] >= e->items)
		return -EINVAL;

	val = snd_soc_enum_item_to_val(e, item[0]) << e->shift_l;
	mask = e->mask << e->shift_l;

	for (i = 0; i < num_codecs; i++) {
		dev_dbg(codec_dais[i]->component->dev,
			"%s: reg = %u, mask = 0x%04x, val = 0x%04x",
			__func__, e->reg, mask, val);
		ret = snd_soc_component_update_bits(codec_dais[i]->component, e->reg, mask,
				val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

// TODO: Add DE-EMPHASIS control

static const char *codec_att2db_texts[] = {"Off", "On"};
static const char *codec_dither_texts[] = {"Off", "RPDF", "TPDF", "HPDF"};
static const char *codec_filter_texts[] = {"Response 1", "Response 2",
		"Response 3", "Response 4", "Response 5"};

static SOC_ENUM_SINGLE_DECL(codec_att2db_enum,
		WM8741_VOLUME_CONTROL, WM8741_ATT2DB_SHIFT, codec_att2db_texts);
static SOC_ENUM_SINGLE_DECL(codec_dither_enum,
		WM8741_MODE_CONTROL_2, WM8741_DITHER_SHIFT, codec_dither_texts);
static SOC_ENUM_SINGLE_DECL(codec_filter_enum,
		WM8741_FILTER_CONTROL, WM8741_FIRSEL_SHIFT, codec_filter_texts);

static const struct snd_kcontrol_new taudac_controls[] = {
	SOC_ENUM_EXT("Anti-Clipping Mode", codec_att2db_enum,
			codec_get_enum, codec_put_enum),
	SOC_ENUM_EXT("Dither", codec_dither_enum,
			codec_get_enum, codec_put_enum),
	SOC_ENUM_EXT("Filter", codec_filter_enum,
			codec_get_enum, codec_put_enum),
};

/*
 * asoc digital audio interface
 */
static int taudac_init(struct snd_soc_pcm_runtime *rtd)
{
	int ret;
	struct snd_soc_card_drvdata *drvdata =
			snd_soc_card_get_drvdata(rtd->card);

	ret = taudac_i2s_clks_init(drvdata);
	if (ret < 0) {
		dev_err(rtd->card->dev,
				"Failed to initialize bit clocks: %d\n", ret);
		return ret;
	}

	ret = taudac_codecs_init(rtd);
	if (ret < 0) {
		dev_err(rtd->card->dev,
				"Failed to configure codecs: %d\n", ret);
		return ret;
	}

	return 0;
}

static void taudac_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card_drvdata *drvdata =
			snd_soc_card_get_drvdata(rtd->card);

	taudac_codecs_shutdown(rtd);
	taudac_i2s_clks_disable(drvdata);
	taudac_mclk_disable(drvdata);
}

static int taudac_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	int ret;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card_drvdata *drvdata =
			snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	unsigned int mclk_rate, bclk_rate;
	unsigned int lrclk_rate = params_rate(params);
	int width = params_width(params);

	unsigned int fmt = SND_SOC_DAIFMT_I2S;
	unsigned int cpu_fmt;
	unsigned int codec_fmt;

	switch (width) {
	case 16:
		fmt |= SND_SOC_DAIFMT_IB_NF;
		break;
	case 24:
		width = 32;
		fallthrough;
	case 32:
		fmt |= SND_SOC_DAIFMT_NB_NF;
		break;
	default:
		dev_err(rtd->card->dev, "Bit depth not supported: %d", width);
		return -EINVAL;
	}

	cpu_fmt   = fmt | SND_SOC_DAIFMT_CBM_CFM;
	codec_fmt = fmt | SND_SOC_DAIFMT_CBS_CFS;

	switch (lrclk_rate) {
	case 44100:
	case 88200:
	case 176400:
		mclk_rate = 22579200;
		break;
	case 32000:
	case 48000:
	case 96000:
	case 192000:
		mclk_rate = 24576000;
		break;
	default:
		dev_err(rtd->card->dev, "Sample rate not supported: %d",
				lrclk_rate);
		return -EINVAL;
	}

	bclk_rate = 2 * width * lrclk_rate;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 2 * width);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, cpu_fmt);
	if (ret < 0)
		return ret;

	/* prepare codecs */
	ret = taudac_codecs_prepare(rtd, mclk_rate, codec_fmt);
	if (ret < 0)
		return ret;

	/* enable clocks */
	ret = taudac_mclk_enable(drvdata, mclk_rate);
	if (ret < 0)
		return ret;

	ret = taudac_i2s_clks_set_rate(drvdata, bclk_rate, lrclk_rate);
	if (ret < 0)
		return ret;

	ret = taudac_i2s_clks_enable(drvdata);
	if (ret < 0)
		return ret;

	/* startup codecs */
	ret = taudac_codecs_startup(rtd);
	if (ret < 0)
		return ret;

	dev_dbg(rtd->card->dev, "%s: mclk = %u, bclk = %u, lrclk = %u, width = %d, fmt = 0x%x",
			__func__, mclk_rate, bclk_rate, lrclk_rate, width, fmt);

	return 0;
}

static struct snd_soc_ops taudac_ops = {
	.hw_params = taudac_hw_params,
	.shutdown  = taudac_shutdown,
};

SND_SOC_DAILINK_DEFS(taudac,
	DAILINK_COMP_ARRAY(COMP_CPU("bcm2708-i2s.0")),
	DAILINK_COMP_ARRAY(COMP_CODEC("wm8741.1-001a", "wm8741"),
	                   COMP_CODEC("wm8741.1-001b", "wm8741")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("bcm2708-i2s.0")));

static struct snd_soc_dai_link taudac_dai[] = {
	{
		.name          = "TauDAC I2S",
		.stream_name   = "TauDAC HiFi",
		.dai_fmt       = SND_SOC_DAIFMT_I2S |
				 SND_SOC_DAIFMT_NB_NF |
				 SND_SOC_DAIFMT_CBS_CFS,
		.playback_only = true,
		.ops  = &taudac_ops,
		.init = taudac_init,
		SND_SOC_DAILINK_REG(taudac),
	},
};

static struct snd_soc_codec_conf taudac_codec_conf[] = {
	{
		.dlc = COMP_CODEC_CONF("wm8741.1-001a"),
		.name_prefix = "Left",
	},
	{
		.dlc = COMP_CODEC_CONF("wm8741.1-001b"),
		.name_prefix = "Right",
	},
};

/*
 * asoc machine driver
 */
static struct snd_soc_card taudac_card = {
	.name         = "TauDAC",
	.dai_link     = taudac_dai,
	.num_links    = ARRAY_SIZE(taudac_dai),
	.codec_conf   = taudac_codec_conf,
	.num_configs  = ARRAY_SIZE(taudac_codec_conf),
	.controls     = taudac_controls,
	.num_controls = ARRAY_SIZE(taudac_controls),
};

/*
 * platform device driver
 */
static int taudac_set_dai(struct device_node *np)
{
	int i;

	struct device_node *i2s_node;
	struct device_node *i2c_nodes[ARRAY_SIZE(taudac_codec_conf)];
	struct snd_soc_dai_link *dai = &taudac_dai[0];

	/* dais */
	i2s_node = of_parse_phandle(np, "taudac,i2s-controller", 0);

	if (i2s_node == NULL)
		return -EINVAL;

	dai->cpus->dai_name = NULL;
	dai->cpus->of_node = i2s_node;
	dai->platforms->name = NULL;
	dai->platforms->of_node = i2s_node;

	for (i = 0; i < ARRAY_SIZE(i2c_nodes); i++) {
		i2c_nodes[i] = of_parse_phandle(np, "taudac,codecs", i);

		if (i2c_nodes[i] == NULL)
			return -EINVAL;

		dai->codecs[i].name = NULL;
		dai->codecs[i].of_node = i2c_nodes[i];
	}

	return 0;
}

static int taudac_set_clk(struct device *dev,
		struct snd_soc_card_drvdata *drvdata)
{
	drvdata->mclk24 = devm_clk_get(dev, "mclk-24M");
	if (IS_ERR(drvdata->mclk24))
		return -EINVAL;

	drvdata->mclk22 = devm_clk_get(dev, "mclk-22M");
	if (IS_ERR(drvdata->mclk22))
		return -EINVAL;

	drvdata->mclk_mux = devm_clk_get(dev, "mux-mclk");
	if (IS_ERR(drvdata->mclk_mux))
		return -EINVAL;

	drvdata->mclk_gate = devm_clk_get(dev, "gate-mclk");
	if (IS_ERR(drvdata->mclk_gate))
		return -EINVAL;

	drvdata->bclk[BCLK_CPU] = devm_clk_get(dev, "bclk-cpu");
	if (IS_ERR(drvdata->bclk[BCLK_CPU]))
		return -EPROBE_DEFER;

	drvdata->bclk[BCLK_DACL] = devm_clk_get(dev, "bclk-dacl");
	if (IS_ERR(drvdata->bclk[BCLK_DACL]))
		return -EPROBE_DEFER;

	drvdata->bclk[BCLK_DACR] = devm_clk_get(dev, "bclk-dacr");
	if (IS_ERR(drvdata->bclk[BCLK_DACR]))
		return -EPROBE_DEFER;

	drvdata->lrclk[LRCLK_CPU] = devm_clk_get(dev, "lrclk-cpu");
	if (IS_ERR(drvdata->lrclk[LRCLK_CPU]))
		return -EPROBE_DEFER;

	drvdata->lrclk[LRCLK_DACL] = devm_clk_get(dev, "lrclk-dacl");
	if (IS_ERR(drvdata->lrclk[LRCLK_DACL]))
		return -EPROBE_DEFER;

	drvdata->lrclk[LRCLK_DACR] = devm_clk_get(dev, "lrclk-dacr");
	if (IS_ERR(drvdata->lrclk[LRCLK_DACR]))
		return -EPROBE_DEFER;

	return 0;
}

static int taudac_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np;
	struct snd_soc_card_drvdata *drvdata;

	taudac_card.dev = &pdev->dev;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL)
		return -ENOMEM;

	np = pdev->dev.of_node;
	if (np == NULL) {
		dev_err(&pdev->dev, "Device tree node not found\n");
		return -ENODEV;
	}

	/* set dai */
	ret = taudac_set_dai(np);
	if (ret != 0) {
		dev_err(&pdev->dev, "Setting dai failed: %d\n", ret);
		return ret;
	}

	/* set clocks */
	ret = taudac_set_clk(&pdev->dev, drvdata);
	if (ret != 0) {
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Getting clocks failed: %d\n", ret);
		return ret;
	}

	/* register card */
	snd_soc_card_set_drvdata(&taudac_card, drvdata);
	snd_soc_of_parse_card_name(&taudac_card, "taudac,model");
	ret = snd_soc_register_card(&taudac_card);
	if (ret != 0) {
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n",
					ret);
		return ret;
	}

	return ret;
}

static int taudac_remove(struct platform_device *pdev)
{
	return snd_soc_unregister_card(&taudac_card);
}

static const struct of_device_id taudac_of_match[] = {
	{ .compatible = "taudac,dm101", },
	{},
};
MODULE_DEVICE_TABLE(of, taudac_of_match);

static struct platform_driver taudac_driver = {
	.driver = {
		.name  = "snd-soc-taudac",
		.owner = THIS_MODULE,
		.of_match_table = taudac_of_match,
	},
	.probe  = taudac_probe,
	.remove = taudac_remove,
};

module_platform_driver(taudac_driver);

MODULE_AUTHOR("Sergej Sawazki <taudac@gmx.de>");
MODULE_DESCRIPTION("ASoC Driver for TauDAC");
MODULE_LICENSE("GPL v2");
