/*
 * ADC_DMA_Config.c
 *
 *  Created on: May 17, 2026
 *      Author: mehmet_dora
 */

#include "ADC_DMA_Config.h"
#include "IIR_DSP.h"
#include "ADC_DMA_Config.h"




#define IIR_ALPHA 0.35f
FirstOrderIIR filt;






void dma2_init(){


	// DMA CONFIG
	RCC->AHB1ENR |= (1UL << 22);		// DMA2 clock enable


	DMA2_Stream0->CR &= ~(1UL << 0);	// Önce disable et, sonra enable olmasını bekle
	while((DMA2_Stream0->CR & (1UL << 0)) != 0){};

	DMA2_Stream0->CR &= ~(3UL << 6);	// DMA2 peripheral to memory selected
	DMA2_Stream0->CR |= (1UL << 8);		// DMA2 circular mode enable
	DMA2_Stream0->CR &= ~(1UL << 9);	// DMA2 peripheral address sabit
	DMA2_Stream0->CR |= (1UL << 10);	// DMA2 memory address artsın
	DMA2_Stream0->CR |= (1UL << 11);	// DMA2 peripheral size 16 bit
	DMA2_Stream0->CR |= (1UL << 13);	// DMA2 memory size 16 bit
	DMA2_Stream0->CR |= (2UL << 16);	// DMA2 priority level high
	DMA2_Stream0->CR &= ~(7UL << 25);	// DMA2 chanel 0

	DMA2_Stream0->NDTR = FFT_SIZE;	// Kaç kere transfer yapılacağı
	DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;	// Peripheral adress
	DMA2_Stream0->M0AR = (uint32_t)adc_buffer;	// Memory adress

	DMA2_Stream0->FCR |= (2UL << 0);	// 3/4 treshold FIFO

	DMA2_Stream0->CR |= (1UL << 4);		// DMA transfer complete interrupt enable


	// DMA2 Stream0 interrupt'ı etkinleştir
	NVIC_SetPriority(DMA2_Stream0_IRQn, 1);
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);


	// ADC, timer ile başlatılacak
	DMA2_Stream0->CR |= (1UL << 0);	// DMA2 başlat

}


void adc1_init(void){

	// GPIO CONFIG
	RCC->AHB1ENR |= (1UL << 0);		// GPIOA clock enable

	GPIOA->MODER |= (3UL << 0);		// PA0 analog mode
	GPIOA->OSPEEDR |= (3UL << 0);	// PA0 high speed




	// ADC CONFIG
	RCC->APB2ENR |= (1UL << 8);		// ADC1 clock enable

	ADC1->CR1 &= ~(1UL << 8);		// tek kanal olduğundan , scan mode disable
	ADC1->CR1 &= ~(3UL << 24);		// resolution -> 12 bit
	ADC1->CR2 |= (1UL << 0);		// ADC1 enable
	ADC1->CR2 &= ~(1UL << 1);		// continuous conversion mode enable
	ADC1->CR2 |= (1UL << 8);		// ADC1 DMA mode enable
	ADC1->CR2 |= (1UL << 9);		// ADC1 DMA enable
	ADC1->CR2 |= (1UL << 10);		// ADC1 EOC enable

	// External trigger enable: rising edge
	ADC1->CR2 &= ~(3UL << 28);
	ADC1->CR2 |=  (1UL << 28);

	// External event select: TIM2 TRGO
	ADC1->CR2 &= ~(15UL << 24);
	ADC1->CR2 |=  (6UL << 24);


	ADC1->SQR3 &= ~(15UL << 0);		// Sequence olarak 0. olarak başlasın

	HAL_Delay(10);

	//FirstOrderIIR_Init(&filt, IIR_ALPHA);
	// IIR filtresi kullanılmayacak artık, raw sinyal FFT tarafından işleniyor
}





void DMA2_Stream0_IRQHandler(void) {


    if(DMA2->LISR & (1UL << 5)) {

        DMA2->LIFCR |= (1UL << 5);  // flag temizle


        for(uint16_t i = 0; i < FFT_SIZE; i++) {


        	//float filtered = FirstOrderIIR_Update(&filt, adc_buffer[i]);
        	/*
			adc_fft_buffer[i] = (uint16_t)(adc_buffer[i] < 0.0f ? 0.0f :
										   adc_buffer[i] > 4095.0f ? 4095.0f :
										   adc_buffer[i]);
			*/

        	/*
        	 * Şimdilik IIR filtresi çıkartıldı çünkü low-pass filtre olduğu için sinyaldeki
        	 * ani high değerleri sönümlüyor. Bu da uzaktaki cisimden gelen zayıf sinyallerin
        	 * daha da sönümlenmesine neden olabilir. FFT ye raw ADC verileri verilere uzaktaki
        	 * zayıf sinyaller içinden zaten frekanslarına göre ayrım yapılıyor.
        	 */






        	adc_fft_buffer[i] = adc_buffer[i];

        	debug_raw_sample = adc_buffer[FFT_SIZE - 1];
        	debug_filtered_sample = adc_fft_buffer[FFT_SIZE - 1];
		}


        fft_ready_flag = 1;
    }

}



