/*
 * imx-max98090.c
 *
 * Copyright (C) 2015 Amazon Lab126
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Based on imx-sgtl5000.c
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 * Copyright (C) 2012 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#define DEBUG
#include <linux/device.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/control.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/pinctrl/consumer.h>

#include "../codecs/max98090.h"
#include "imx-audmux.h"

#define DAI_NAME_SIZE	32

struct imx_max98090_data {
	struct snd_soc_dai_link dai;
	struct snd_soc_card card;
	char codec_dai_name[DAI_NAME_SIZE];
	char platform_name[DAI_NAME_SIZE];
	unsigned int clk_frequency;
};

struct imx_priv {
	int mic_gpio;
	int mic_active_low;
	struct snd_soc_codec *codec;
	struct platform_device *pdev;
	struct snd_pcm_substream *first_stream;
	struct snd_pcm_substream *second_stream;
	/* TODO: Enable kcontrol
	struct snd_kcontrol *lineIn_kctl;
	*/
	struct snd_card *snd_card;
};
static struct imx_priv card_priv;

static struct snd_soc_jack imx_mic_jack;
static struct snd_soc_jack_pin imx_mic_jack_pins[] = {
	{
		.pin = "AMIC",
		.mask = SND_JACK_MICROPHONE,
	},
	{
		.pin = "Line Jack",
		.mask = SND_JACK_MICROPHONE,
	},
};

/* TODO: Enable with gpio pins
static struct snd_soc_jack_gpio imx_mic_jack_gpio = {
	.name = "microphone detect",
	.report = SND_JACK_MICROPHONE,
	.debounce_time = 250,
	.invert = 0,
};
*/
static int micjack_status_check(void)
{
	struct imx_priv *priv = &card_priv;
	struct platform_device *pdev = priv->pdev;
	char *envp[3], *buf;
	int mic_status, ret;

	pr_info("%s:imx-max98090 enter\n", __func__);

	if (!gpio_is_valid(priv->mic_gpio))
		return 0;

	mic_status = gpio_get_value(priv->mic_gpio) ? 1 : 0;

	buf = kmalloc(32, GFP_ATOMIC);
	if (!buf) {
		pr_err("%s:imx-max98090 kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s:imx-max98090 mic_status=%d, active_low=%d\n",
		__func__, mic_status, priv->mic_active_low);

	if (mic_status != priv->mic_active_low) {
		snprintf(buf, 32, "STATE=%d", 2);
		/* TODO: Enable with DAPM
		snd_soc_dapm_disable_pin(&priv->codec->dapm, "AMIC");
		ret = imx_mic_jack_gpio.report;
		snd_kctl_jack_report(priv->snd_card, priv->lineIn_kctl, 1);
		*/
	} else {
		snprintf(buf, 32, "STATE=%d", 0);
		/* TODO: Enable with DAPM
		snd_soc_dapm_enable_pin(&priv->codec->dapm, "AMIC");
		ret = 0;
		snd_kctl_jack_report(priv->snd_card, priv->lineIn_kctl, 0);
		*/
	}

	envp[0] = "NAME=microphone";
	envp[1] = buf;
	envp[2] = NULL;
	kobject_uevent_env(&pdev->dev.kobj, KOBJ_CHANGE, envp);
	kfree(buf);

	return ret;
}


static const struct snd_soc_dapm_widget imx_max98090_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Main SpeakerL", NULL),
	SND_SOC_DAPM_SPK("Main SpeakerR", NULL),
	SND_SOC_DAPM_MIC("AMIC", NULL),
	SND_SOC_DAPM_LINE("Line Jack", NULL),
};

static int imx_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct imx_priv *priv = &card_priv;
	struct snd_soc_card *card = codec_dai->codec->card;
	struct imx_max98090_data *data = snd_soc_card_get_drvdata(card);
	unsigned int sample_rate = params_rate(params);
	snd_pcm_format_t sample_format = params_format(params);
	u32 dai_format, pll_out;
	int ret = 0;

	pr_info("%s:imx-max98090 stream name '%s'\n", __func__,
		substream->name);

	if (!priv->first_stream) {
		priv->first_stream = substream;
	} else {
		priv->second_stream = substream;
		pr_info("%s:imx-max98090 second stream\n", __func__);
		/* We suppose the two substream are using same params */
		return 0;
	}

	/* Master mode for MAX98090 right here */
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBM_CFM;


	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_format);
	if (ret) {
		pr_err("%s:imx-max98090 failed to set codec dai fmt: %d\n",
			__func__, ret);
		return ret;
	}

	/* TODO: Enable if neccessary
	ret = snd_soc_dai_set_tdm_slot(codec_dai, 0x10, 0, 1, 24);
	if (ret) {
		pr_err("%s:imx-max98090 failed to set DAI TDM slot: %d\n",
		__func__, ret);
		return ret;
	}
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_format);
	if (ret) {
		pr_err("%s:imx-max98090 failed to set cpu dai fmt: %d\n",
		__func__, ret);
		return ret;
	}
	*/

	/* set the codec system clock for DAC and ADC to xtal value */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, data->clk_frequency,
				SND_SOC_CLOCK_IN);
	if (ret < 0) {
		pr_err("%s:imx-max98090 failed to set codec system clock %d\n",
		__func__, ret);
		return ret;
	}

	if (sample_format == SNDRV_PCM_FORMAT_S24_LE)
		pll_out = sample_rate * 384;
	else
		pll_out = sample_rate * 256;


	/* TODO: Enable sets sysclk for cpu_dai
	ret = snd_soc_dai_set_sysclk(cpu_dai, OMAP_MCBSP_CLKR_SRC_CLKX, 0,
		SND_SOC_CLOCK_IN);
	if (ret < 0) {
		pr_err("can't set CPU system clock OMAP_MCBSP_CLKR_SRC_CLKX\n");
		return ret;
	}

	*/

	return 0;
}

static int imx_hifi_hw_free(struct snd_pcm_substream *substream)
{
	struct imx_priv *priv = &card_priv;

	pr_info("%s:imx-max98090 enter\n", __func__);

	/* We don't need to handle anything if there's no substream running */
	if (!priv->first_stream)
		return 0;

	if (priv->first_stream == substream)
		priv->first_stream = priv->second_stream;
	priv->second_stream = NULL;

	if (!priv->first_stream) {
		/*
		 * Continuously setting FLL would cause playback distortion.
		 * We can fix it just by mute codec after playback.
		 */

		/* TODO: Enable with DAPM
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			snd_soc_dai_digital_mute(codec_dai, 1,
			substream->stream);
		*/
		/* Disable FLL and let codec do pm_runtime_put() */
	}

	return 0;
}

static struct snd_soc_ops imx_hifi_ops = {
	.hw_params = imx_hifi_hw_params,
	.hw_free = imx_hifi_hw_free,
};

static int imx_max98090_gpio_init(struct snd_soc_card *card)
{
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dai;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct imx_priv *priv = &card_priv;

	pr_info("%s:imx-max98090 enter\n", __func__);

	priv->codec = codec;

	/* TODO: Enable Jack handling here
	if (gpio_is_valid(priv->mic_gpio)) {
		imx_mic_jack_gpio.gpio = priv->mic_gpio;
		imx_mic_jack_gpio.jack_status_check = micjack_status_check;
		*/

		snd_soc_jack_new(codec, "JACK", SND_JACK_MICROPHONE,
				 &imx_mic_jack);
		snd_soc_jack_add_pins(&imx_mic_jack,
				ARRAY_SIZE(imx_mic_jack_pins),
				      imx_mic_jack_pins);
		/*snd_soc_jack_add_gpios(&imx_mic_jack, 1, &imx_mic_jack_gpio);
		*/
		/* Configure mic detect */
		max98090_mic_detect(codec, &imx_mic_jack);
	/*
	}*/

	return 0;
}

static ssize_t show_mic(struct device_driver *dev, char *buf)
{
	struct imx_priv *priv = &card_priv;
	int jack_status = 0;

	pr_info("%s:imx-max98090 enter\n", __func__);

	jack_status = max98090_read_jack(priv->codec);

	if (jack_status < 0) {
		strcpy(buf, "Failed to get Jack Status\n");
	return strlen(buf);
	}

	pr_info("%s:imx-max98090 Jack=%x\n", __func__, jack_status);

	jack_status &= (M98090_LSNS_MASK|M98090_JKSNS_MASK);

	/* amic when no line connected */
	if (jack_status == (M98090_LSNS_MASK|M98090_JKSNS_MASK))
		strcpy(buf, "amic\n");
	/* dmic when microphone connected */
	else if (jack_status == M98090_JKSNS_MASK)
		strcpy(buf, "dmic\n");
	/* line In*/
	else
		strcpy(buf, "line\n");

	return strlen(buf);
}

static DRIVER_ATTR(microphone, S_IRUGO | S_IWUSR, show_mic, NULL);

static int imx_max98090_late_probe(struct snd_soc_card *card)
{
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dai;
	struct imx_max98090_data *data = snd_soc_card_get_drvdata(card);
	int ret;

	pr_info("%s:imx-max98090 enter\n", __func__);

	ret = snd_soc_dai_set_sysclk(codec_dai, 0,
		data->clk_frequency, SND_SOC_CLOCK_IN);
	if (ret < 0)
		pr_err("%s:imx-max98090 failed to set sysclk\n", __func__);

	return ret;
}

static const struct snd_kcontrol_new controls[] = {
	SOC_DAPM_PIN_SWITCH("Main Speaker"),
	SOC_DAPM_PIN_SWITCH("DMIC"),
};

static int imx_max98090_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *cpu_np, *codec_np;
	struct platform_device *cpu_pdev;
	struct imx_priv *priv = &card_priv;
	struct i2c_client *codec_dev;
	struct imx_max98090_data *data;
	struct clk *codec_clk = NULL;
	int int_port, ext_port;
	int ret;

	pr_info("%s:imx-max98090 pdev->name %s, num_resources %d\n", __func__,
		pdev->name, pdev->num_resources);


	priv->pdev = pdev;

	cpu_np = of_parse_phandle(pdev->dev.of_node, "cpu-dai", 0);
	if (!cpu_np) {
		pr_err("%s:imx-max98090 cpu dai phandle missing or invalid\n",
			__func__);
		ret = -EINVAL;
		goto fail;
	}

	if (!strstr(cpu_np->name, "ssi"))
		goto audmux_bypass;

	pr_info("%s:imx-max98090 snd AUDMUX programming\n", __func__);

	ret = of_property_read_u32(np, "mux-int-port", &int_port);
	if (ret) {
		pr_err("%s:imx-max98090 mux-int-port missing or invalid\n",
			__func__);
		return ret;
	}
	ret = of_property_read_u32(np, "mux-ext-port", &ext_port);
	if (ret) {
		pr_err("%s:imx-max98090 mux-ext-port missing or invalid\n",
			__func__);
		return ret;
	}

	/*
	 * The port numbering in the hardware manual starts at 1, while
	 * the audmux API expects it starts at 0.
	 */
	int_port--;
	ext_port--;
	ret = imx_audmux_v2_configure_port(int_port,
		IMX_AUDMUX_V2_PTCR_SYN |
		IMX_AUDMUX_V2_PTCR_TFSEL(ext_port) |
		IMX_AUDMUX_V2_PTCR_TCSEL(ext_port) |
		IMX_AUDMUX_V2_PTCR_TFSDIR |
		IMX_AUDMUX_V2_PTCR_TCLKDIR,
		IMX_AUDMUX_V2_PDCR_RXDSEL(ext_port));

	if (ret) {
		pr_err("%s:imx-max98090 audmux internal port setup failed %d\n",
			__func__, ret);
		return ret;
	}

	imx_audmux_v2_configure_port(ext_port,
		IMX_AUDMUX_V2_PTCR_SYN,
		IMX_AUDMUX_V2_PDCR_RXDSEL(int_port));

	if (ret) {
		pr_err("%s:imx-max98090 audmux external port setup failed %d\n",
			__func__, ret);
		return ret;
	}
audmux_bypass:
	codec_np = of_parse_phandle(pdev->dev.of_node, "audio-codec", 0);
	if (!codec_np) {
		pr_err("%s:imx-max98090 phandle missing or invalid\n",
			__func__);
		ret = -EINVAL;
		goto fail;
	}

	cpu_pdev = of_find_device_by_node(cpu_np);
	if (!cpu_pdev) {
		pr_err("%s:imx-max98090 failed to find SSI platform device\n",
			__func__);
		ret = -EINVAL;
		goto fail;
	}
	codec_dev = of_find_i2c_device_by_node(codec_np);
	if (!codec_dev || !codec_dev->driver) {
		pr_err("%s:imx-max98090 failed to find codec platform device\n",
			__func__);
		ret = -EINVAL;
		goto fail;
	}

	priv->first_stream = NULL;
	priv->second_stream = NULL;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto fail;
	}

	codec_clk = devm_clk_get(&codec_dev->dev, NULL);
	if (IS_ERR(codec_clk)) {
		ret = PTR_ERR(codec_clk);
		pr_err("%s:imx-max98090 failed to get codec clk: %d\n",
			__func__, ret);
		goto fail;
	}

	data->clk_frequency = clk_get_rate(codec_clk);
	/* TODO: Change clock rate if needed
	clk_set_rate(mclk, freq);
	*/
	ret = clk_prepare_enable(codec_clk);
	if (ret < 0)
		pr_err("%s:imx-max98090 failed to clk_prepare_enable: %d\n",
			__func__, ret);
	else
		pr_info("%s:imx-max98090 clock clk_prepare_enable(%d)\n",
			__func__, data->clk_frequency);

	priv->mic_gpio = of_get_named_gpio_flags(np, "mic-det-gpios", 0,
				(enum of_gpio_flags *)&priv->mic_active_low);

	data->dai.name = "max98090";
	data->dai.stream_name = "max98090";
	data->dai.codec_dai_name = "max98090-HiFi";
	data->dai.codec_of_node = codec_np;
	data->dai.cpu_dai_name = dev_name(&cpu_pdev->dev);
	data->dai.platform_of_node = cpu_np;
	data->dai.ops = &imx_hifi_ops;
	data->dai.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBM_CFM;

	pr_info("%s:imx-max98090 Codec CLk=%d, GPIO=%d\n", __func__,
		data->clk_frequency, priv->mic_gpio);

	data->card.dev = &pdev->dev;
	ret = snd_soc_of_parse_card_name(&data->card, "model");
	if (ret) {
		pr_err("%s:imx-max98090 parse_card_name failed (%d)\n",
			__func__, ret);
		goto fail;
	}
	ret = snd_soc_of_parse_audio_routing(&data->card, "audio-routing");
	if (ret) {
		pr_err("%s:imx-max98090 parse_audio_routing failed (%d)\n",
			__func__, ret);
		goto fail;
	}
	data->card.num_links = 1;
	data->card.dai_link = &data->dai;
	data->card.dapm_widgets = imx_max98090_dapm_widgets;
	data->card.num_dapm_widgets = ARRAY_SIZE(imx_max98090_dapm_widgets);

	data->card.late_probe = imx_max98090_late_probe;

	platform_set_drvdata(pdev, &data->card);
	snd_soc_card_set_drvdata(&data->card, data);

	ret = snd_soc_register_card(&data->card);
	if (ret) {
		pr_err("%s:imx-max98090 snd_soc_register_card failed (%d)\n",
			__func__, ret);
		goto fail;
	}

	priv->snd_card = data->card.snd_card;
	/* TODO: Enable with kcontrol
	priv->lineIn_kctl = snd_kctl_jack_new("Line Jack", 0, NULL);
	ret = snd_ctl_add(data->card.snd_card, priv->lineIn_kctl);
	if (ret)
		goto fail;
	*/

	imx_max98090_gpio_init(&data->card);

	ret = driver_create_file(pdev->dev.driver, &driver_attr_microphone);
	if (ret) {
		pr_err("%s:imx-max98090 create mic attr failed (%d)\n",
			__func__, ret);
		goto fail_mic;
	}

	goto fail;

fail_mic:
	snd_soc_unregister_card(&data->card);
fail:
	if (cpu_np)
		of_node_put(cpu_np);
	if (codec_np)
		of_node_put(codec_np);

	return ret;
}

static int imx_max98090_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	pr_info("%s:imx-max98090 snd\n", __func__);

	driver_remove_file(pdev->dev.driver, &driver_attr_microphone);

	snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id imx_max98090_dt_ids[] = {
	{ .compatible = "fsl,imx-audio-max98090", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_max98090_dt_ids);

static struct platform_driver imx_max98090_driver = {
	.driver = {
		.name = "imx-max98090",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = imx_max98090_dt_ids,
	},
	.probe = imx_max98090_probe,
	.remove = imx_max98090_remove,
};
module_platform_driver(imx_max98090_driver);


MODULE_AUTHOR("Amazon Lab126 Inc.");
MODULE_DESCRIPTION("Amazon Lab126 i.MX MAX98090 ASoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:imx-max98090");
