/*
 * TimerDriver.c
 *
 *  Created on: May 17, 2026
 *      Author: mehmet_dora
 */

#include "TimerDriver.h"
#include "stm32f4xx.h"


// Sample rate değeri ADC nin örnekleme hızı demektir
void TimerDriver_init(uint32_t sampling_rate){

	// Timer 2 için Clock Enable
	RCC->APB1ENR |= (1UL << 0);


	TIM2->PSC = 83;

	// Frekansı belirleyen yer
	TIM2->ARR = (1000000 / sampling_rate) - 1;




	// 4. TRGO
	/*
	 * Bu TRGO ayarı ile normalde yazılımsal olarak adc nin başlatılması ve çalışması yerine
	 * timer döngüleri ile başlatılması sağlanması için kullanılıyor. Yani her timer döngüsü
	 * sonunda tetikleme sinyali yayınlanıyor, adc bu sinyalin gelmesi ile birlikte conversion
	 * yapıyor ve duruyor. Her tetikleme gelmesi ile adc çalışıyor, yani timer ın çalışma hızına
	 * göre adc conversion yapmış olması sağlanıyor.
	 */
	TIM2->CR2 &= ~(7UL << 4);
	TIM2->CR2 |= (2UL << 4);

	// timer başlatma
	TIM2->CR1 |= (1UL << 0);





}
