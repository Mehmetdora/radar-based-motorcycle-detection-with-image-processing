/*
 * ADC_DMA_Config.h
 *
 *  Created on: May 17, 2026
 *      Author: mehmet_dora
 */

#ifndef MODULES_ADC_DMA_CONFIG_H_
#define MODULES_ADC_DMA_CONFIG_H_

#include "stm32f4xx.h"
#include <stdint.h>
#include "FFT_DSP.h"




extern uint16_t adc_buffer[FFT_SIZE];
extern uint16_t adc_fft_buffer[FFT_SIZE];
extern volatile uint8_t fft_ready_flag;


extern volatile float debug_raw_sample;
extern volatile float debug_filtered_sample;



void dma2_init(void);
void adc1_init(void);

#endif /* MODULES_ADC_DMA_CONFIG_H_ */
