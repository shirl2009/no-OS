/***************************************************************************//**
 *   @file   app.c
 *   @brief  AD9081 application example.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdio.h>
#include <inttypes.h>
#include "error.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "spi.h"
#include "spi_extra.h"
#include "ad9081.h"
#include "app_clock.h"
#include "app_jesd.h"
#include "axi_jesd204_rx.h"
#include "axi_jesd204_tx.h"
#include "axi_adc_core.h"
#include "axi_dac_core.h"
#include "axi_dmac.h"
#include "app_parameters.h"
#include "app_config.h"

#ifdef IIO_SUPPORT
#include "app_iio.h"
#endif

extern struct axi_jesd204_rx *rx_jesd;
extern struct axi_jesd204_tx *tx_jesd;

#define BENCHMARK

#include "timer.h"
#include "timer_extra.h"
#include <xil_cache.h>

int main(void)
{
	struct clk app_clk[MULTIDEVICE_INSTANCE_COUNT];
	struct clk jesd_clk[2];
	struct xil_gpio_init_param  xil_gpio_param = {
#ifdef PLATFORM_MB
		.type = GPIO_PL,
#else
		.type = GPIO_PS,
#endif
		.device_id = GPIO_DEVICE_ID
	};
	struct gpio_init_param	gpio_phy_resetb = {
		.number = PHY_RESET,
		.platform_ops = &xil_gpio_platform_ops,
		.extra = &xil_gpio_param
	};
	struct xil_spi_init_param xil_spi_param = {
#ifdef PLATFORM_MB
		.type = SPI_PL,
#else
		.type = SPI_PS,
#endif
		.device_id = PHY_SPI_DEVICE_ID,
	};
	struct spi_init_param phy_spi_init_param = {
		.max_speed_hz = 1000000,
		.mode = SPI_MODE_0,
		.chip_select = PHY_CS,
		.platform_ops = &xil_platform_ops,
		.extra = &xil_spi_param
	};
	struct link_init_param jesd_tx_link = {
		.device_id = 0,
		.octets_per_frame = AD9081_TX_JESD_F,
		.frames_per_multiframe = AD9081_TX_JESD_K,
		.samples_per_converter_per_frame = AD9081_TX_JESD_S,
		.high_density = AD9081_TX_JESD_HD,
		.converter_resolution = AD9081_TX_JESD_N,
		.bits_per_sample = AD9081_TX_JESD_NP,
		.converters_per_device = AD9081_TX_JESD_M,
		.control_bits_per_sample = AD9081_TX_JESD_CS,
		.lanes_per_device = AD9081_TX_JESD_L,
		.subclass = AD9081_TX_JESD_SUBCLASS,
		.link_mode = AD9081_TX_JESD_MODE,
		.dual_link = 0,
		.version = AD9081_TX_JESD_VERSION,
		.logical_lane_mapping = AD9081_TX_LOGICAL_LANE_MAPPING,
		.tpl_phase_adjust = 12
	};
	struct link_init_param jesd_rx_link = {
		.device_id = 0,
		.octets_per_frame = AD9081_RX_JESD_F,
		.frames_per_multiframe = AD9081_RX_JESD_K,
		.samples_per_converter_per_frame = AD9081_RX_JESD_S,
		.high_density = AD9081_RX_JESD_HD,
		.converter_resolution = AD9081_RX_JESD_N,
		.bits_per_sample = AD9081_RX_JESD_NP,
		.converters_per_device = AD9081_RX_JESD_M,
		.control_bits_per_sample = AD9081_RX_JESD_CS,
		.lanes_per_device = AD9081_RX_JESD_L,
		.subclass = AD9081_RX_JESD_SUBCLASS,
		.link_mode = AD9081_RX_JESD_MODE,
		.dual_link = 0,
		.version = AD9081_RX_JESD_VERSION,
		.logical_lane_mapping = AD9081_RX_LOGICAL_LANE_MAPPING,
		.link_converter_select = AD9081_RX_LINK_CONVERTER_SELECT,
	};
	struct ad9081_init_param phy_param = {
		.gpio_reset = &gpio_phy_resetb,
		.spi_init = &phy_spi_init_param,
		.dev_clk = &app_clk[0],
		.jesd_tx_clk = &jesd_clk[1],
		.jesd_rx_clk = &jesd_clk[0],
		.multidevice_instance_count = 1,
#ifdef QUAD_MXFE
		.jesd_sync_pins_01_swap_enable = true,
#else
		.jesd_sync_pins_01_swap_enable = false,
#endif
		.lmfc_delay_dac_clk_cycles = 0,
		.nco_sync_ms_extra_lmfc_num = 0,
		/* TX */
		.dac_frequency_hz = AD9081_DAC_FREQUENCY,
		/* The 4 DAC Main Datapaths */
		.tx_main_interpolation = AD9081_TX_MAIN_INTERPOLATION,
		.tx_main_nco_frequency_shift_hz = AD9081_TX_MAIN_NCO_SHIFT,
		.tx_dac_channel_crossbar_select = AD9081_TX_DAC_CHAN_CROSSBAR,
		/* The 8 DAC Channelizers */
		.tx_channel_interpolation = AD9081_TX_CHAN_INTERPOLATION,
		.tx_channel_nco_frequency_shift_hz = AD9081_TX_CHAN_NCO_SHIFT,
		.tx_channel_gain = AD9081_TX_CHAN_GAIN,
		.jesd_tx_link = &jesd_tx_link,
		/* RX */
		.adc_frequency_hz = AD9081_ADC_FREQUENCY,
		.nyquist_zone = AD9081_ADC_NYQUIST_ZONE,
		/* The 4 ADC Main Datapaths */
		.rx_main_nco_frequency_shift_hz = AD9081_RX_MAIN_NCO_SHIFT,
		.rx_main_decimation = AD9081_RX_MAIN_DECIMATION,
		.rx_main_complex_to_real_enable = {0, 0, 0, 0},
		.rx_main_enable = AD9081_RX_MAIN_ENABLE,
		/* The 8 ADC Channelizers */
		.rx_channel_nco_frequency_shift_hz = AD9081_RX_CHAN_NCO_SHIFT,
		.rx_channel_decimation = AD9081_RX_CHAN_DECIMATION,
		.rx_channel_complex_to_real_enable = {0, 0, 0, 0, 0, 0, 0, 0},
		.rx_channel_enable = AD9081_RX_CHAN_ENABLE,
		.jesd_rx_link[0] = &jesd_rx_link,
		.jesd_rx_link[1] = NULL,
	};

	struct axi_adc_init rx_adc_init = {
		.name = "rx_adc",
		.base = RX_CORE_BASEADDR
	};
	struct axi_adc *rx_adc;
	struct axi_dac_init tx_dac_init = {
		.name = "tx_dac",
		.base = TX_CORE_BASEADDR,
		.channels = NULL
	};
	struct axi_dac *tx_dac;
	struct axi_dmac_init rx_dmac_init = {
		"rx_dmac",
		RX_DMA_BASEADDR,
		DMA_DEV_TO_MEM,
		0
	};
	struct axi_dmac *rx_dmac;
	struct axi_dmac_init tx_dmac_init = {
		"tx_dmac",
		TX_DMA_BASEADDR,
		DMA_MEM_TO_DEV,
		DMA_CYCLIC
	};
	struct axi_dmac *tx_dmac;
	struct ad9081_phy* phy[MULTIDEVICE_INSTANCE_COUNT];
	int32_t status;
	int32_t i;

	printf("Hello\n");

#ifdef QUAD_MXFE
	struct xil_gpio_init_param  xil_gpio_param_2 = {
#ifdef PLATFORM_MB
		.type = GPIO_PL,
#else
		.type = GPIO_PS,
#endif
		.device_id = GPIO_2_DEVICE_ID
	};
	struct gpio_init_param	ad9081_gpio0_mux_init = {
		.number = AD9081_GPIO_0_MUX,
		.platform_ops = &xil_gpio_platform_ops,
		.extra = &xil_gpio_param_2
	};
	gpio_desc *ad9081_gpio0_mux;

	status = gpio_get(&ad9081_gpio0_mux, &ad9081_gpio0_mux_init);
	if (status)
		return status;

	status = gpio_set_value(ad9081_gpio0_mux, 1);
	if (status)
		return status;
#endif

	status = app_clock_init(app_clk);
	if (status != SUCCESS)
		printf("app_clock_init() error: %" PRId32 "\n", status);

	status = app_jesd_init(jesd_clk,
			       500000, 250000, 250000, 10000000, 10000000);
	if (status != SUCCESS)
		printf("app_jesd_init() error: %" PRId32 "\n", status);

	rx_adc_init.num_channels = 0;
	tx_dac_init.num_channels = 0;

	for (i = 0; i < MULTIDEVICE_INSTANCE_COUNT; i++) {
		gpio_phy_resetb.number = PHY_RESET + i;
		phy_spi_init_param.chip_select = PHY_CS + i;
		phy_param.dev_clk = &app_clk[i];
		jesd_rx_link.device_id = i;

		status = ad9081_init(&phy[i], &phy_param);
		if (status != SUCCESS)
			printf("ad9081_init() error: %" PRId32 "\n", status);

		rx_adc_init.num_channels += phy[i]->jesd_rx_link[0].jesd_param.jesd_m +
					    phy[i]->jesd_rx_link[1].jesd_param.jesd_m;

		tx_dac_init.num_channels += phy[i]->jesd_tx_link.jesd_param.jesd_m *
					    (phy[i]->jesd_tx_link.jesd_param.jesd_duallink > 0 ? 2 : 1);
	}

	axi_jesd204_rx_watchdog(rx_jesd);

	axi_jesd204_tx_status_read(tx_jesd);
	axi_jesd204_rx_status_read(rx_jesd);

	axi_dac_init(&tx_dac, &tx_dac_init);
	axi_adc_init(&rx_adc, &rx_adc_init);

	axi_dmac_init(&tx_dmac, &tx_dmac_init);
	axi_dmac_init(&rx_dmac, &rx_dmac_init);

#ifdef BENCHMARK
	struct timer_desc *timer;
	struct xil_timer_init_param xil_timer_param = {
		.active_tmr = 0,
		.type = TIMER_PL
	};

	struct timer_init_param timer_param = {
		.id = XPAR_AXI_TIMER2_DEVICE_ID,
		.freq_hz = XPAR_AXI_TIMER2_CLOCK_FREQ_HZ,
		.load_value = 0,
		.extra = &xil_timer_param
	};
	uint32_t counter_in;
	uint32_t counter_out;
	uint32_t counter_diff;
	uint32_t freq_hz;

	timer_init(&timer, &timer_param);
	timer_count_clk_get(timer, &freq_hz);
	printf("Timer frequency: %lu\n", freq_hz);

	/* Coarse NCO DDC update */
	uint8_t cddc;

	cddc = ad9081_get_main_adc_mask(phy[0], 0);
	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	adi_ad9081_adc_ddc_coarse_nco_set(&phy[0]->ad9081, cddc, 100000000);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("Coarse NCO DDC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	uint8_t coarse_nco_ddc[][3] = {
		{0x80, 0x18, 0x00},	// r
		{0x00, 0x18, 0x1d},
		{0x80, 0x18, 0x8d},	// r
		{0x00, 0x18, 0x1d},
		{0x0a, 0x05, 0x66},
		{0x0a, 0x06, 0x66},
		{0x0a, 0x07, 0x66},
		{0x0a, 0x08, 0x66},
		{0x0a, 0x09, 0x66},
		{0x0a, 0x0a, 0x06},
		{0x0a, 0x11, 0x00},
		{0x0a, 0x12, 0x00},
		{0x0a, 0x13, 0x00},
		{0x0a, 0x14, 0x00},
		{0x0a, 0x15, 0x00},
		{0x0a, 0x16, 0x00},
		{0x0a, 0x17, 0x00},
		{0x0a, 0x18, 0x00},
		{0x0a, 0x19, 0x00},
		{0x0a, 0x1a, 0x00},
		{0x0a, 0x1b, 0x00},
		{0x0a, 0x1c, 0x00},
		{0x80, 0x18, 0x1d},	// r
		{0x00, 0x18, 0x1d},
		{0x0a, 0x0b, 0x30},
		{0x0a, 0x0c, 0x33},
		{0x0a, 0x0d, 0x33},
		{0x0a, 0x0e, 0x33},
		{0x0a, 0x0f, 0x33},
		{0x0a, 0x10, 0x33},
	};

	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	for (i = 0; i < sizeof(coarse_nco_ddc) / 3; i++)
		spi_write_and_read(phy[0]->spi_desc, coarse_nco_ddc[i], 3);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("No API - Coarse NCO DDC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	/* Coarse NCO DUC update */
	for (i = 0; i < ARRAY_SIZE(phy[0]->tx_dac_chan_xbar);
	     i++) {
		if (phy[0]->tx_dac_chan_xbar[i] &
		    BIT(0)) {
			timer_start(timer);
			timer_counter_set(timer, 0xFFFFFFFF);
			counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
			adi_ad9081_dac_duc_nco_set(
				&phy[0]->ad9081, BIT(i),
				AD9081_DAC_CH_NONE, 100000000);
			counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
			timer_stop(timer);
			counter_diff = counter_in - counter_out;
			printf("Coarse NCO DUC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);
		}
	}

	uint8_t dac_duc_nco[][3] = {
		{0x00, 0x1b, 0x01},
		{0x81, 0xc9, 0x08},	// r
		{0x01, 0xc9, 0x08},
		{0x00, 0x1b, 0x01},
		{0x81, 0xca, 0x03},	// r
		{0x01, 0xca, 0x03},
		{0x81, 0xc9, 0x08},	// r
		{0x01, 0xc9, 0x08},
		{0x00, 0x1b, 0x01},
		{0x01, 0xcb, 0x22},
		{0x01, 0xcc, 0x22},
		{0x01, 0xcd, 0x22},
		{0x01, 0xce, 0x22},
		{0x01, 0xcf, 0x22},
		{0x01, 0xd0, 0x02},
		{0x81, 0xca, 0x03},	// r
		{0x01, 0xca, 0x02},
		{0x81, 0xca, 0x00},	// r
		{0x01, 0xca, 0x01},
		{0x81, 0xca, 0x03},	// r
	};

	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	for (i = 0; i < sizeof(dac_duc_nco) / 3; i++)
		spi_write_and_read(phy[0]->spi_desc, dac_duc_nco[i], 3);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("No API: Coarse NCO DUC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	/* Fine NCO DDC update */
	uint8_t fddc_num;

	fddc_num = phy[0]->jesd_rx_link[0].link_converter_select[0] / 2;
	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	adi_ad9081_adc_ddc_fine_nco_set(&phy[0]->ad9081, BIT(fddc_num), 100000000);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("Fine NCO DDC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	uint8_t fine_nco_ddc[][3] = {
		{0x82, 0x81, 0xaa},	// r
		{0x80, 0x18, 0x1d},	// r
		{0x00, 0x18, 0x1d},
		{0x82, 0x82, 0x01},	// r
		{0x82, 0x82, 0x01},	// r
		{0x00, 0x19, 0x01},
		{0x0a, 0x85, 0x99},
		{0x0a, 0x86, 0x99},
		{0x0a, 0x87, 0x99},
		{0x0a, 0x88, 0x99},
		{0x0a, 0x89, 0x99},
		{0x0a, 0x8a, 0x19},
		{0x0a, 0x91, 0x00},
		{0x0a, 0x92, 0x00},
		{0x0a, 0x93, 0x00},
		{0x0a, 0x94, 0x00},
		{0x0a, 0x95, 0x00},
		{0x0a, 0x96, 0x00},
		{0x0a, 0x97, 0x00},
		{0x0a, 0x98, 0x00},
		{0x0a, 0x99, 0x00},
		{0x0a, 0x9a, 0x00},
		{0x0a, 0x9b, 0x00},
		{0x0a, 0x9c, 0x00},
	};

	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	for (i = 0; i < sizeof(fine_nco_ddc) / 3; i++)
		spi_write_and_read(phy[0]->spi_desc, fine_nco_ddc[i], 3);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("No API: Fine NCO DDC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	/* Fine NCO DUC update */
	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	adi_ad9081_dac_duc_nco_set(&phy[0]->ad9081,
				 AD9081_DAC_NONE,
				 BIT(0),
				 100000000);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("Fine NCO DUC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	uint8_t fine_nco_duc[][3] = {
		{0x81, 0xff, 0x68},	// r
		{0x00, 0x1c, 0x01},
		{0x81, 0xa0, 0x40},	// r
		{0x01, 0xa0, 0x40},
		{0x00, 0x1c, 0x01},
		{0x81, 0xa1, 0x03},	// r
		{0x01, 0xa1, 0x03},
		{0x81, 0xa0, 0x40},	// r
		{0x01, 0xa0, 0x40},
		{0x00, 0x1c, 0x01},
		{0x01, 0xa2, 0xcc},
		{0x01, 0xa3, 0xcc},
		{0x01, 0xa4, 0xcc},
		{0x01, 0xa5, 0xcc},
		{0x01, 0xa6, 0xcc},
		{0x01, 0xa7, 0x0c},
		{0x81, 0xa1, 0x03},	// r
		{0x01, 0xa1, 0x02},
		{0x81, 0xa1, 0x00},	// r
		{0x01, 0xa1, 0x01},
		{0x81, 0xa1, 0x03},	// r
	};

	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	for (i = 0; i < sizeof(fine_nco_duc) / 3; i++)
		spi_write_and_read(phy[0]->spi_desc, fine_nco_duc[i], 3);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("No API: Fine NCO DUC update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	/* PFIR update */
	uint16_t coeffs[192];

	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	counter_in = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	adi_ad9081_adc_pfir_config_set(&phy[0]->ad9081,
			AD9081_ADC_PFIR_ADC_PAIR_ALL,
			AD9081_ADC_PFIR_COEFF_PAGE_ALL,
			AD9081_ADC_PFIR_I_MODE_COMPLEX_FULL,
			AD9081_ADC_PFIR_Q_MODE_COMPLEX_FULL,
			AD9081_ADC_PFIR_GAIN_N6DB,
			AD9081_ADC_PFIR_GAIN_N6DB,
			AD9081_ADC_PFIR_GAIN_N6DB,
			AD9081_ADC_PFIR_GAIN_N6DB,
			BIT(4),
			coeffs,
			192);
	counter_out = Xil_In32(XPAR_AXI_TIMER2_BASEADDR + XTC_TCR_OFFSET);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("PFIR update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);

	uint8_t pfir_update[][3] = {
		{0x00, 0x1e, 0x01},
		{0x8c, 0x0c, 0x00},	// r
		{0x0c, 0x0c, 0x05},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x0c, 0x00},	// r
		{0x0c, 0x0c, 0x05},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x0c, 0x05},	// r
		{0x0c, 0x0c, 0x55},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x0c, 0x05},	// r
		{0x0c, 0x0c, 0x55},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x0d, 0x00},	// r
		{0x0c, 0x0d, 0x07},
		{0x8c, 0x0d, 0x07},	// r
		{0x0c, 0x0d, 0x3f},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x0d, 0x00},	// r
		{0x0c, 0x0d, 0x07},
		{0x8c, 0x0d, 0x07},	// r
		{0x0c, 0x0d, 0x3f},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x0f, 0x00},	// r
		{0x0c, 0x0f, 0x07},
		{0x8c, 0x0f, 0x07},	// r
		{0x0c, 0x0f, 0x3f},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x0f, 0x00},	// r
		{0x0c, 0x0f, 0x07},
		{0x8c, 0x0f, 0x07},	// r
		{0x0c, 0x0f, 0x3f},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x1d, 0x00},	// r
		{0x0c, 0x1d, 0x10},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x1d, 0x00},	// r
		{0x0c, 0x1d, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x00, 0x00},
		{0x19, 0x01, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x02, 0x73},
		{0x19, 0x03, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x04, 0x00},
		{0x19, 0x05, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x06, 0xf8},
		{0x19, 0x07, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x08, 0x1c},
		{0x19, 0x09, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x0a, 0xc1},
		{0x19, 0x0b, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x0c, 0x00},
		{0x19, 0x0d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x0e, 0xbb},
		{0x19, 0x0f, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x10, 0x03},
		{0x19, 0x11, 0xd0},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x12, 0x56},
		{0x19, 0x13, 0x16},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x14, 0x0c},
		{0x19, 0x15, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x16, 0xb2},
		{0x19, 0x17, 0xbe},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x18, 0x00},
		{0x19, 0x19, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x1a, 0x16},
		{0x19, 0x1b, 0x11},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x1c, 0x00},
		{0x19, 0x1d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x1e, 0x1a},
		{0x19, 0x1f, 0x11},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x20, 0x00},
		{0x19, 0x21, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x22, 0x77},
		{0x19, 0x23, 0xe8},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x24, 0x30},
		{0x19, 0x25, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x26, 0x63},
		{0x19, 0x27, 0xe8},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x28, 0x00},
		{0x19, 0x29, 0x18},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x2a, 0xfc},
		{0x19, 0x2b, 0x99},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x2c, 0x00},
		{0x19, 0x2d, 0x40},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x2e, 0x18},
		{0x19, 0x2f, 0x13},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x30, 0x00},
		{0x19, 0x31, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x32, 0x63},
		{0x19, 0x33, 0x12},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x34, 0x00},
		{0x19, 0x35, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x36, 0xb7},
		{0x19, 0x37, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x38, 0x00},
		{0x19, 0x39, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x3a, 0xc3},
		{0x19, 0x3b, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x3c, 0xff},
		{0x19, 0x3d, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x3e, 0x00},
		{0x19, 0x3f, 0xb0},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x40, 0x44},
		{0x19, 0x41, 0xfa},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x42, 0xf4},
		{0x19, 0x43, 0xb9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x44, 0x00},
		{0x19, 0x45, 0xb0},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x46, 0xc3},
		{0x19, 0x47, 0x16},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x48, 0x58},
		{0x19, 0x49, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x4a, 0x53},
		{0x19, 0x4b, 0xbe},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x4c, 0x00},
		{0x19, 0x4d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x4e, 0x73},
		{0x19, 0x4f, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x50, 0x94},
		{0x19, 0x51, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x52, 0x19},
		{0x19, 0x53, 0xbe},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x54, 0x28},
		{0x19, 0x55, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x56, 0x01},
		{0x19, 0x57, 0xe9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x58, 0x00},
		{0x19, 0x59, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x5a, 0xfe},
		{0x19, 0x5b, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x5c, 0x00},
		{0x19, 0x5d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x5e, 0xdd},
		{0x19, 0x5f, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x60, 0x1c},
		{0x19, 0x61, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x62, 0xa1},
		{0x19, 0x63, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x64, 0xfb},
		{0x19, 0x65, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x66, 0x00},
		{0x19, 0x67, 0xb0},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x68, 0x98},
		{0x19, 0x69, 0xd8},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x6a, 0xf4},
		{0x19, 0x6b, 0xb9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x6c, 0x00},
		{0x19, 0x6d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x6e, 0x3c},
		{0x19, 0x6f, 0x13},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x70, 0x00},
		{0x19, 0x71, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x72, 0xb7},
		{0x19, 0x73, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x74, 0xff},
		{0x19, 0x75, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x76, 0x00},
		{0x19, 0x77, 0xb0},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x78, 0xc0},
		{0x19, 0x79, 0xf9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x7a, 0xf4},
		{0x19, 0x7b, 0xb9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x7c, 0x1c},
		{0x19, 0x7d, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x7e, 0xc1},
		{0x19, 0x7f, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x80, 0x00},
		{0x19, 0x81, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x82, 0x77},
		{0x19, 0x83, 0xe8},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x84, 0x00},
		{0x19, 0x85, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x86, 0xd3},
		{0x19, 0x87, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x88, 0x04},
		{0x19, 0x89, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x8a, 0x63},
		{0x19, 0x8b, 0xe8},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x8c, 0x00},
		{0x19, 0x8d, 0x18},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x8e, 0xfc},
		{0x19, 0x8f, 0x99},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x90, 0x1c},
		{0x19, 0x91, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x92, 0xa1},
		{0x19, 0x93, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x94, 0x74},
		{0x19, 0x95, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x96, 0x36},
		{0x19, 0x97, 0xbe},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x98, 0x00},
		{0x19, 0x99, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x9a, 0xf8},
		{0x19, 0x9b, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x9c, 0x00},
		{0x19, 0x9d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0x9e, 0x73},
		{0x19, 0x9f, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xa0, 0x00},
		{0x19, 0xa1, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xa2, 0xe1},
		{0x19, 0xa3, 0xe9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xa4, 0x2c},
		{0x19, 0xa5, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xa6, 0x61},
		{0x19, 0xa7, 0xea},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xa8, 0x30},
		{0x19, 0xa9, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xaa, 0xc1},
		{0x19, 0xab, 0xea},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xac, 0x34},
		{0x19, 0xad, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xae, 0xe1},
		{0x19, 0xaf, 0xea},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xb0, 0x38},
		{0x19, 0xb1, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xb2, 0x01},
		{0x19, 0xb3, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xb4, 0x3c},
		{0x19, 0xb5, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xb6, 0x21},
		{0x19, 0xb7, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xb8, 0x40},
		{0x19, 0xb9, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xba, 0x41},
		{0x19, 0xbb, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xbc, 0x44},
		{0x19, 0xbd, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xbe, 0x61},
		{0x19, 0xbf, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xc0, 0x48},
		{0x19, 0xc1, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xc2, 0x81},
		{0x19, 0xc3, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xc4, 0x4c},
		{0x19, 0xc5, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xc6, 0xa1},
		{0x19, 0xc7, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xc8, 0x50},
		{0x19, 0xc9, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xca, 0xc1},
		{0x19, 0xcb, 0xeb},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xcc, 0x08},
		{0x19, 0xcd, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xce, 0x0f},
		{0x19, 0xcf, 0xb6},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xd0, 0x54},
		{0x19, 0xd1, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xd2, 0x21},
		{0x19, 0xd3, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xd4, 0xdc},
		{0x19, 0xd5, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xd6, 0x21},
		{0x19, 0xd7, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xd8, 0x20},
		{0x19, 0xd9, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xda, 0xc1},
		{0x19, 0xdb, 0xfa},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xdc, 0x00},
		{0x19, 0xdd, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xde, 0xe1},
		{0x19, 0xdf, 0xf9},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xe0, 0x1c},
		{0x19, 0xe1, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xe2, 0x61},
		{0x19, 0xe3, 0xfa},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xe4, 0x00},
		{0x19, 0xe5, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xe6, 0x65},
		{0x19, 0xe7, 0xe8},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xe8, 0x40},
		{0x19, 0xe9, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xea, 0x63},
		{0x19, 0xeb, 0xea},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xec, 0x14},
		{0x19, 0xed, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xee, 0x13},
		{0x19, 0xef, 0xbe},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xf0, 0x00},
		{0x19, 0xf1, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xf2, 0xc5},
		{0x19, 0xf3, 0x12},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xf4, 0x00},
		{0x19, 0xf5, 0x98},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xf6, 0xfc},
		{0x19, 0xf7, 0x99},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xf8, 0x00},
		{0x19, 0xf9, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xfa, 0xa6},
		{0x19, 0xfb, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xfc, 0x00},
		{0x19, 0xfd, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x19, 0xfe, 0x63},
		{0x19, 0xff, 0x12},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x00, 0x00},
		{0x1a, 0x01, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x02, 0xd3},
		{0x1a, 0x03, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x04, 0xff},
		{0x1a, 0x05, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x06, 0x00},
		{0x1a, 0x07, 0xb0},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x08, 0x7c},
		{0x1a, 0x09, 0xf9},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x0a, 0xf4},
		{0x1a, 0x0b, 0xb9},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x0c, 0x00},
		{0x1a, 0x0d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x0e, 0xb6},
		{0x1a, 0x0f, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x10, 0x00},
		{0x1a, 0x11, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x12, 0x73},
		{0x1a, 0x13, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x14, 0x00},
		{0x1a, 0x15, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x16, 0xe1},
		{0x1a, 0x17, 0xe9},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x18, 0x1c},
		{0x1a, 0x19, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x1a, 0x61},
		{0x1a, 0x1b, 0xea},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x1c, 0x20},
		{0x1a, 0x1d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x1e, 0xc1},
		{0x1a, 0x1f, 0xea},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x20, 0x08},
		{0x1a, 0x21, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x22, 0x0f},
		{0x1a, 0x23, 0xb6},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x24, 0x24},
		{0x1a, 0x25, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x26, 0x21},
		{0x1a, 0x27, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x28, 0xbc},
		{0x1a, 0x29, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x2a, 0x21},
		{0x1a, 0x2b, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x2c, 0x38},
		{0x1a, 0x2d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x2e, 0x41},
		{0x1a, 0x2f, 0xfb},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x30, 0x00},
		{0x1a, 0x31, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x32, 0x46},
		{0x1a, 0x33, 0x13},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x34, 0x3c},
		{0x1a, 0x35, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x36, 0x61},
		{0x1a, 0x37, 0xfb},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x38, 0x00},
		{0x1a, 0x39, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x3a, 0x65},
		{0x1a, 0x3b, 0x13},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x3c, 0x00},
		{0x1a, 0x3d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x3e, 0xba},
		{0x1a, 0x3f, 0x10},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x40, 0x20},
		{0x1a, 0x41, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x42, 0xc0},
		{0x1a, 0x43, 0x30},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x44, 0x30},
		{0x1a, 0x45, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x46, 0x01},
		{0x1a, 0x47, 0xfb},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x48, 0x00},
		{0x1a, 0x49, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x4a, 0xe1},
		{0x1a, 0x4b, 0xf9},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x4c, 0x24},
		{0x1a, 0x4d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x4e, 0x61},
		{0x1a, 0x4f, 0xfa},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x50, 0x28},
		{0x1a, 0x51, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x52, 0xc1},
		{0x1a, 0x53, 0xfa},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x54, 0x2c},
		{0x1a, 0x55, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x56, 0xe1},
		{0x1a, 0x57, 0xfa},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x58, 0x34},
		{0x1a, 0x59, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x5a, 0x21},
		{0x1a, 0x5b, 0xfb},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x5c, 0x40},
		{0x1a, 0x5d, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x5e, 0x81},
		{0x1a, 0x5f, 0xfb},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x60, 0xfb},
		{0x1a, 0x61, 0xff},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x62, 0x00},
		{0x1a, 0x63, 0xb0},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x64, 0x64},
		{0x1a, 0x65, 0xd9},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x66, 0xf4},
		{0x1a, 0x67, 0xb9},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x68, 0x00},
		{0x1a, 0x69, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x6a, 0x07},
		{0x1a, 0x6b, 0x13},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x6c, 0x98},
		{0x1a, 0x6d, 0x01},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x6e, 0x03},
		{0x1a, 0x6f, 0xbe},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x70, 0x20},
		{0x1a, 0x71, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x72, 0x61},
		{0x1a, 0x73, 0xf8},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x74, 0x01},
		{0x1a, 0x75, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x76, 0xe3},
		{0x1a, 0x77, 0x32},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x78, 0x50},
		{0x1a, 0x79, 0x5b},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x7a, 0x04},
		{0x1a, 0x7b, 0x00},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x7c, 0xd0},
		{0x1a, 0x7d, 0x5b},
		{0x00, 0x1f, 0x0f},
		{0x1a, 0x7e, 0x04},
		{0x1a, 0x7f, 0x00},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x1d, 0x10},	// r
		{0x0c, 0x1d, 0x00},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x1d, 0x10},	// r
		{0x0c, 0x1d, 0x00},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x17, 0x00},	// r
		{0x0c, 0x17, 0x01},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x17, 0x00},	// r
		{0x0c, 0x17, 0x01},
		{0x00, 0x1e, 0x01},
		{0x8c, 0x17, 0x01},	// r
		{0x0c, 0x17, 0x00},
		{0x00, 0x1e, 0x02},
		{0x8c, 0x17, 0x01},	// r
		{0x0c, 0x17, 0x00},
	};

	timer_start(timer);
	timer_counter_set(timer, 0xFFFFFFFF);
	timer_counter_get(timer, &counter_in);
	for (i = 0; i < sizeof(pfir_update) / 3; i++)
		spi_write_and_read(phy[0]->spi_desc, pfir_update[i], 3);
	timer_counter_get(timer, &counter_out);
	timer_stop(timer);
	counter_diff = counter_in - counter_out;
	printf("No API: PFIR update: %d.%05d ms\n", counter_diff / 100000, counter_diff % 100000);
#endif

#ifdef IIO_SUPPORT
	printf("The board accepts libiio clients connections through the serial backend.\n");

	struct iio_axi_adc_init_param iio_axi_adc_init_par;
	iio_axi_adc_init_par = (struct iio_axi_adc_init_param) {
		.rx_adc = rx_adc,
		.rx_dmac = rx_dmac,
	};

	struct iio_axi_dac_init_param iio_axi_dac_init_par;
	iio_axi_dac_init_par = (struct iio_axi_dac_init_param) {
		.tx_dac = tx_dac,
		.tx_dmac = tx_dmac,
	};

	return iio_server_init(&iio_axi_adc_init_par, &iio_axi_dac_init_par);
#else
	printf("Bye\n");

	return SUCCESS;
#endif

}
