/*
 * fsl_asrc.h - ALSA ASRC interface
 *
 * Copyright (C) 2013-2014 Freescale Semiconductor, Inc.  This file is licensed
 * under the terms of the GNU General Public License version 2.  This
 * program is licensed "as is" without any warranty of any kind, whether
 * express or implied.
 */

#ifndef _FSL_ASRC_P2P_H
#define _FSL_ASRC_P2P_H

#include <linux/mxc_asrc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/platform_data/dma-imx.h>

enum peripheral_device_type {
	UNKNOWN,
	SSI1,
	SSI2,
	SSI3,
	ESAI,
};

struct fsl_asrc_p2p_params {
	enum asrc_pair_index asrc_index;
	struct dma_async_tx_descriptor  *desc;
	struct dma_chan                 *dma_chan;
	struct imx_dma_data              dma_data;
};

struct fsl_asrc_p2p {
	int p2p_rate;
	int p2p_width;
	enum peripheral_device_type per_dev;
	struct asrc_p2p_ops asrc_ops;

	struct snd_dmaengine_dai_dma_data dma_params_rx;
	struct snd_dmaengine_dai_dma_data dma_params_tx;
	struct imx_dma_data filter_data_tx;
	struct imx_dma_data filter_data_rx;
	struct snd_pcm_substream *substream[2];

	/* playback and record p2p params */
	struct fsl_asrc_p2p_params p2p_params[2];
	struct platform_device *soc_platform_pdev;

	int dmarx[3];
	int dmatx[3];

	char name[32];
};

#endif
