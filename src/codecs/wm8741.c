/*
 * wm8741.c  --  WM8741 ALSA SoC Audio driver
 *
 * Copyright 2010-1 Wolfson Microelectronics plc
 *
 * Author: Ian Lartey <ian@opensource.wolfsonmicro.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "wm8741.h"

#define WM8741_NUM_SUPPLIES 2
static const char *wm8741_supply_names[WM8741_NUM_SUPPLIES] = {
	"AVDD",
	"DVDD",
};

/* codec private data */
struct wm8741_priv {
	struct wm8741_platform_data pdata;
	struct regmap *regmap;
	struct regulator_bulk_data supplies[WM8741_NUM_SUPPLIES];
	unsigned int sysclk;
	const struct snd_pcm_hw_constraint_list *sysclk_constraints;
};

static const struct reg_default wm8741_reg_defaults[] = {
	{  0, 0x0000 },     /* R0  - DACLLSB Attenuation */
	{  1, 0x0000 },     /* R1  - DACLMSB Attenuation */
	{  2, 0x0000 },     /* R2  - DACRLSB Attenuation */
	{  3, 0x0000 },     /* R3  - DACRMSB Attenuation */
	{  4, 0x0000 },     /* R4  - Volume Control */
	{  5, 0x000A },     /* R5  - Format Control */
	{  6, 0x0000 },     /* R6  - Filter Control */
	{  7, 0x0000 },     /* R7  - Mode Control 1 */
	{  8, 0x0002 },     /* R8  - Mode Control 2 */
	{ 32, 0x0002 },     /* R32 - ADDITONAL_CONTROL_1 */
};

static unsigned int _wm8741_volume_get(struct snd_soc_codec *codec,
		struct soc_mixer_control *mc, unsigned int lsb_reg)
{
	unsigned int msb_reg = lsb_reg + 1;
	unsigned int mask = mc->max;
	unsigned int val = (snd_soc_read(codec, lsb_reg) |
			(snd_soc_read(codec, msb_reg) << mc->shift)) & mask;

	if (mc->invert)
		val = mc->max - val;

	return val;
}

static int _wm8741_volume_set(struct snd_soc_codec *codec,
		struct soc_mixer_control *mc, unsigned int lsb_reg,
		unsigned int val)
{
	unsigned int msb_reg = lsb_reg + 1;
	unsigned int upd = (1 << mc->shift);
	unsigned int mask = upd - 1;

	if (mc->invert)
		val = mc->max - val;

	snd_soc_write(codec, lsb_reg, val & mask);
	snd_soc_write(codec, msb_reg, (val >> mc->shift) | upd);

	return 0;
}

static int wm8741_reset(struct snd_soc_codec *codec)
{
	return snd_soc_write(codec, WM8741_RESET, 0);
}

static int wm8741_volume_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;

	ucontrol->value.integer.value[0] =
			_wm8741_volume_get(codec, mc, mc->reg);

	if (snd_soc_volsw_is_stereo(mc))
		ucontrol->value.integer.value[1] =
				_wm8741_volume_get(codec, mc, mc->rreg);

	return 0;
}

static int wm8741_volume_set(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;

	_wm8741_volume_set(codec, mc, mc->reg,
			ucontrol->value.integer.value[0]);

	if (snd_soc_volsw_is_stereo(mc))
		_wm8741_volume_set(codec, mc, mc->rreg,
				ucontrol->value.integer.value[1]);

	return 0;
}

static const DECLARE_TLV_DB_MINMAX_MUTE(wm8741_vol_tlv, -12788, 0);

static const struct snd_kcontrol_new wm8741_snd_controls_stereo[] = {
SOC_DOUBLE_R_EXT_TLV("Playback Volume",
		WM8741_DACLLSB_ATTENUATION, WM8741_DACRLSB_ATTENUATION,
		WM8741_UPDATELL_SHIFT, 0x3ff, 1,
		wm8741_volume_get, wm8741_volume_set, wm8741_vol_tlv),
};

static const struct snd_kcontrol_new wm8741_snd_controls_mono_left[] = {
SOC_SINGLE_EXT_TLV("Playback Volume", WM8741_DACLLSB_ATTENUATION,
		WM8741_UPDATELL_SHIFT, 0x3ff, 1,
		wm8741_volume_get, wm8741_volume_set, wm8741_vol_tlv),
};

static const struct snd_kcontrol_new wm8741_snd_controls_mono_right[] = {
SOC_SINGLE_EXT_TLV("Playback Volume", WM8741_DACRLSB_ATTENUATION,
		WM8741_UPDATERL_SHIFT, 0x3ff, 1,
		wm8741_volume_get, wm8741_volume_set, wm8741_vol_tlv),
};

static const struct snd_soc_dapm_widget wm8741_dapm_widgets[] = {
SND_SOC_DAPM_DAC("DACL", "Playback", SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_DAC("DACR", "Playback", SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_OUTPUT("VOUTLP"),
SND_SOC_DAPM_OUTPUT("VOUTLN"),
SND_SOC_DAPM_OUTPUT("VOUTRP"),
SND_SOC_DAPM_OUTPUT("VOUTRN"),
};

static const struct snd_soc_dapm_route wm8741_dapm_routes[] = {
	{ "VOUTLP", NULL, "DACL" },
	{ "VOUTLN", NULL, "DACL" },
	{ "VOUTRP", NULL, "DACR" },
	{ "VOUTRN", NULL, "DACR" },
};

static const unsigned int rates_11289[] = {
	44100, 88200,
};

static const struct snd_pcm_hw_constraint_list constraints_11289 = {
	.count	= ARRAY_SIZE(rates_11289),
	.list	= rates_11289,
};

static const unsigned int rates_12288[] = {
	32000, 48000, 96000,
};

static const struct snd_pcm_hw_constraint_list constraints_12288 = {
	.count	= ARRAY_SIZE(rates_12288),
	.list	= rates_12288,
};

static const unsigned int rates_16384[] = {
	32000,
};

static const struct snd_pcm_hw_constraint_list constraints_16384 = {
	.count	= ARRAY_SIZE(rates_16384),
	.list	= rates_16384,
};

static const unsigned int rates_16934[] = {
	44100, 88200,
};

static const struct snd_pcm_hw_constraint_list constraints_16934 = {
	.count	= ARRAY_SIZE(rates_16934),
	.list	= rates_16934,
};

static const unsigned int rates_18432[] = {
	48000, 96000,
};

static const struct snd_pcm_hw_constraint_list constraints_18432 = {
	.count	= ARRAY_SIZE(rates_18432),
	.list	= rates_18432,
};

static const unsigned int rates_22579[] = {
	44100, 88200, 176400
};

static const struct snd_pcm_hw_constraint_list constraints_22579 = {
	.count	= ARRAY_SIZE(rates_22579),
	.list	= rates_22579,
};

static const unsigned int rates_24576[] = {
	32000, 48000, 96000, 192000
};

static const struct snd_pcm_hw_constraint_list constraints_24576 = {
	.count	= ARRAY_SIZE(rates_24576),
	.list	= rates_24576,
};

static const unsigned int rates_36864[] = {
	48000, 96000, 192000
};

static const struct snd_pcm_hw_constraint_list constraints_36864 = {
	.count	= ARRAY_SIZE(rates_36864),
	.list	= rates_36864,
};

static int wm8741_startup(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);

	if (wm8741->sysclk)
		snd_pcm_hw_constraint_list(substream->runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				wm8741->sysclk_constraints);

	return 0;
}

static int wm8741_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);
	unsigned int iface;
	int i;

	/* The set of sample rates that can be supported depends on the
	 * MCLK supplied to the CODEC - enforce this.
	 */
	if (!wm8741->sysclk) {
		dev_err(codec->dev,
			"No MCLK configured, call set_sysclk() on init or in hw_params\n");
		return -EINVAL;
	}

	/* Find a supported LRCLK rate */
	for (i = 0; i < wm8741->sysclk_constraints->count; i++) {
		if (wm8741->sysclk_constraints->list[i] == params_rate(params))
			break;
	}

	if (i == wm8741->sysclk_constraints->count) {
		dev_err(codec->dev, "LRCLK %d unsupported with MCLK %d\n",
			params_rate(params), wm8741->sysclk);
		return -EINVAL;
	}

	/* bit size */
	switch (params_width(params)) {
	case 16:
		iface = 0x0;
		break;
	case 20:
		iface = 0x1;
		break;
	case 24:
		iface = 0x2;
		break;
	case 32:
		iface = 0x3;
		break;
	default:
		dev_dbg(codec->dev, "wm8741_hw_params:    Unsupported bit size param = %d",
			params_width(params));
		return -EINVAL;
	}

	dev_dbg(codec->dev, "wm8741_hw_params:    bit size param = %d, rate param = %d",
		params_width(params), params_rate(params));

	snd_soc_update_bits(codec, WM8741_FORMAT_CONTROL, WM8741_IWL_MASK,
			    iface);

	return 0;
}

static int wm8741_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);

	dev_dbg(codec->dev, "wm8741_set_dai_sysclk info: freq=%dHz\n", freq);

	switch (freq) {
	case 0:
		wm8741->sysclk_constraints = NULL;
		break;
	case 11289600:
		wm8741->sysclk_constraints = &constraints_11289;
		break;
	case 12288000:
		wm8741->sysclk_constraints = &constraints_12288;
		break;
	case 16384000:
		wm8741->sysclk_constraints = &constraints_16384;
		break;
	case 16934400:
		wm8741->sysclk_constraints = &constraints_16934;
		break;
	case 18432000:
		wm8741->sysclk_constraints = &constraints_18432;
		break;
	case 22579200:
	case 33868800:
		wm8741->sysclk_constraints = &constraints_22579;
		break;
	case 24576000:
		wm8741->sysclk_constraints = &constraints_24576;
		break;
	case 36864000:
		wm8741->sysclk_constraints = &constraints_36864;
		break;
	default:
		return -EINVAL;
	}

	wm8741->sysclk = freq;
	return 0;
}

static int wm8741_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned int iface;

	/* check master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface = 0x08;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface = 0x00;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface = 0x04;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface = 0x0C;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface = 0x1C;
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x10;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x20;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x30;
		break;
	default:
		return -EINVAL;
	}


	dev_dbg(codec->dev, "wm8741_set_dai_fmt:    Format=%x, Clock Inv=%x\n",
				fmt & SND_SOC_DAIFMT_FORMAT_MASK,
				((fmt & SND_SOC_DAIFMT_INV_MASK)));

	snd_soc_update_bits(codec, WM8741_FORMAT_CONTROL,
			    WM8741_BCP_MASK | WM8741_LRP_MASK | WM8741_FMT_MASK,
			    iface);

	return 0;
}

int wm8741_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	dev_dbg(codec->dev, "%s:    mute = %d\n", __func__, mute);

	snd_soc_update_bits(codec, WM8741_VOLUME_CONTROL,
			WM8741_SOFT_MASK, !!mute << WM8741_SOFT_SHIFT);
	return 0;
}

#define WM8741_RATES (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
			SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | \
			SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 | \
			SNDRV_PCM_RATE_192000)

#define WM8741_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops wm8741_dai_ops = {
	.startup	= wm8741_startup,
	.hw_params	= wm8741_hw_params,
	.set_sysclk	= wm8741_set_dai_sysclk,
	.set_fmt	= wm8741_set_dai_fmt,
	.digital_mute   = wm8741_mute,
};

static struct snd_soc_dai_driver wm8741_dai = {
	.name = "wm8741",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = WM8741_RATES,
		.formats = WM8741_FORMATS,
	},
	.ops = &wm8741_dai_ops,
};

#ifdef CONFIG_PM
static int wm8741_resume(struct snd_soc_codec *codec)
{
	snd_soc_cache_sync(codec);
	return 0;
}
#else
#define wm8741_resume NULL
#endif

static int wm8741_configure(struct snd_soc_codec *codec)
{
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);

	/* Configure differential mode */
	switch (wm8741->pdata.diff_mode) {
	case WM8741_DIFF_MODE_STEREO:
	case WM8741_DIFF_MODE_STEREO_REVERSED:
	case WM8741_DIFF_MODE_MONO_LEFT:
	case WM8741_DIFF_MODE_MONO_RIGHT:
		snd_soc_update_bits(codec, WM8741_MODE_CONTROL_2,
				WM8741_DIFF_MASK,
				wm8741->pdata.diff_mode << WM8741_DIFF_SHIFT);
		break;
	default:
		return -EINVAL;
	}

	/* Change some default settings - latch VU */
	snd_soc_update_bits(codec, WM8741_DACLLSB_ATTENUATION,
			WM8741_UPDATELL, WM8741_UPDATELL);
	snd_soc_update_bits(codec, WM8741_DACLMSB_ATTENUATION,
			WM8741_UPDATELM, WM8741_UPDATELM);
	snd_soc_update_bits(codec, WM8741_DACRLSB_ATTENUATION,
			WM8741_UPDATERL, WM8741_UPDATERL);
	snd_soc_update_bits(codec, WM8741_DACRMSB_ATTENUATION,
			WM8741_UPDATERM, WM8741_UPDATERM);

	return 0;
}

static int wm8741_add_controls(struct snd_soc_codec *codec)
{
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);

	switch (wm8741->pdata.diff_mode) {
	case WM8741_DIFF_MODE_STEREO:
	case WM8741_DIFF_MODE_STEREO_REVERSED:
		snd_soc_add_codec_controls(codec,
				wm8741_snd_controls_stereo,
				ARRAY_SIZE(wm8741_snd_controls_stereo));
		break;
	case WM8741_DIFF_MODE_MONO_LEFT:
		snd_soc_add_codec_controls(codec,
				wm8741_snd_controls_mono_left,
				ARRAY_SIZE(wm8741_snd_controls_mono_left));
		break;
	case WM8741_DIFF_MODE_MONO_RIGHT:
		snd_soc_add_codec_controls(codec,
				wm8741_snd_controls_mono_right,
				ARRAY_SIZE(wm8741_snd_controls_mono_right));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8741_probe(struct snd_soc_codec *codec)
{
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(wm8741->supplies),
				    wm8741->supplies);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to enable supplies: %d\n", ret);
		goto err_get;
	}

	ret = wm8741_reset(codec);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to issue reset\n");
		goto err_enable;
	}

	ret = wm8741_configure(codec);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to change default settings\n");
		goto err_enable;
	}

	ret = wm8741_add_controls(codec);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to add controls\n");
		goto err_enable;
	}

	dev_dbg(codec->dev, "Successful registration\n");
	return ret;

err_enable:
	regulator_bulk_disable(ARRAY_SIZE(wm8741->supplies), wm8741->supplies);
err_get:
	return ret;
}

static int wm8741_remove(struct snd_soc_codec *codec)
{
	struct wm8741_priv *wm8741 = snd_soc_codec_get_drvdata(codec);

	regulator_bulk_disable(ARRAY_SIZE(wm8741->supplies), wm8741->supplies);

	return 0;
}

static const struct snd_soc_codec_driver soc_codec_dev_wm8741 = {
	.probe =	wm8741_probe,
	.remove =	wm8741_remove,
	.resume =	wm8741_resume,

	.component_driver = {
		.dapm_widgets		= wm8741_dapm_widgets,
		.num_dapm_widgets	= ARRAY_SIZE(wm8741_dapm_widgets),
		.dapm_routes		= wm8741_dapm_routes,
		.num_dapm_routes	= ARRAY_SIZE(wm8741_dapm_routes),
	},
};

static const struct of_device_id wm8741_of_match[] = {
	{ .compatible = "wlf,wm8741", },
	{ }
};
MODULE_DEVICE_TABLE(of, wm8741_of_match);

static const struct regmap_config wm8741_regmap = {
	.reg_bits = 7,
	.val_bits = 9,
	.max_register = WM8741_MAX_REGISTER,

	.reg_defaults = wm8741_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(wm8741_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int wm8741_set_pdata(struct device *dev, struct wm8741_priv *wm8741)
{
	const struct wm8741_platform_data *pdata = dev_get_platdata(dev);
	u32 diff_mode;

	if (dev->of_node) {
		if (of_property_read_u32(dev->of_node, "diff-mode", &diff_mode)
				>= 0)
			wm8741->pdata.diff_mode = diff_mode;
	} else {
		if (pdata != NULL)
			memcpy(&wm8741->pdata, pdata, sizeof(wm8741->pdata));
	}

	return 0;
}

#if IS_ENABLED(CONFIG_I2C)
static int wm8741_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct wm8741_priv *wm8741;
	int ret, i;

	wm8741 = devm_kzalloc(&i2c->dev, sizeof(struct wm8741_priv),
			      GFP_KERNEL);
	if (wm8741 == NULL)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(wm8741->supplies); i++)
		wm8741->supplies[i].supply = wm8741_supply_names[i];

	ret = devm_regulator_bulk_get(&i2c->dev, ARRAY_SIZE(wm8741->supplies),
				      wm8741->supplies);
	if (ret != 0) {
		dev_err(&i2c->dev, "Failed to request supplies: %d\n", ret);
		return ret;
	}

	wm8741->regmap = devm_regmap_init_i2c(i2c, &wm8741_regmap);
	if (IS_ERR(wm8741->regmap)) {
		ret = PTR_ERR(wm8741->regmap);
		dev_err(&i2c->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	ret = wm8741_set_pdata(&i2c->dev, wm8741);
	if (ret != 0) {
		dev_err(&i2c->dev, "Failed to set pdata: %d\n", ret);
		return ret;
	}

	i2c_set_clientdata(i2c, wm8741);

	ret = snd_soc_register_codec(&i2c->dev,
				     &soc_codec_dev_wm8741, &wm8741_dai, 1);

	return ret;
}

static int wm8741_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id wm8741_i2c_id[] = {
	{ "wm8741", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wm8741_i2c_id);

static struct i2c_driver wm8741_i2c_driver = {
	.driver = {
		.name = "wm8741",
		.of_match_table = wm8741_of_match,
	},
	.probe =    wm8741_i2c_probe,
	.remove =   wm8741_i2c_remove,
	.id_table = wm8741_i2c_id,
};
#endif

#if defined(CONFIG_SPI_MASTER)
static int wm8741_spi_probe(struct spi_device *spi)
{
	struct wm8741_priv *wm8741;
	int ret, i;

	wm8741 = devm_kzalloc(&spi->dev, sizeof(struct wm8741_priv),
			     GFP_KERNEL);
	if (wm8741 == NULL)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(wm8741->supplies); i++)
		wm8741->supplies[i].supply = wm8741_supply_names[i];

	ret = devm_regulator_bulk_get(&spi->dev, ARRAY_SIZE(wm8741->supplies),
				      wm8741->supplies);
	if (ret != 0) {
		dev_err(&spi->dev, "Failed to request supplies: %d\n", ret);
		return ret;
	}

	wm8741->regmap = devm_regmap_init_spi(spi, &wm8741_regmap);
	if (IS_ERR(wm8741->regmap)) {
		ret = PTR_ERR(wm8741->regmap);
		dev_err(&spi->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	ret = wm8741_set_pdata(&spi->dev, wm8741);
	if (ret != 0) {
		dev_err(&spi->dev, "Failed to set pdata: %d\n", ret);
		return ret;
	}

	spi_set_drvdata(spi, wm8741);

	ret = snd_soc_register_codec(&spi->dev,
			&soc_codec_dev_wm8741, &wm8741_dai, 1);
	return ret;
}

static int wm8741_spi_remove(struct spi_device *spi)
{
	snd_soc_unregister_codec(&spi->dev);
	return 0;
}

static struct spi_driver wm8741_spi_driver = {
	.driver = {
		.name	= "wm8741",
		.of_match_table = wm8741_of_match,
	},
	.probe		= wm8741_spi_probe,
	.remove		= wm8741_spi_remove,
};
#endif /* CONFIG_SPI_MASTER */

static int __init wm8741_modinit(void)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_I2C)
	ret = i2c_add_driver(&wm8741_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register WM8741 I2C driver: %d\n", ret);
#endif
#if defined(CONFIG_SPI_MASTER)
	ret = spi_register_driver(&wm8741_spi_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register wm8741 SPI driver: %d\n",
		       ret);
	}
#endif

	return ret;
}
module_init(wm8741_modinit);

static void __exit wm8741_exit(void)
{
#if defined(CONFIG_SPI_MASTER)
	spi_unregister_driver(&wm8741_spi_driver);
#endif
#if IS_ENABLED(CONFIG_I2C)
	i2c_del_driver(&wm8741_i2c_driver);
#endif
}
module_exit(wm8741_exit);

MODULE_DESCRIPTION("ASoC WM8741 driver");
MODULE_AUTHOR("Ian Lartey <ian@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
