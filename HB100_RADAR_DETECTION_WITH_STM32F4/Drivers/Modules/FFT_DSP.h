/*
 * FFT_DSP.h
 *
 *  Created on: May 17, 2026
 *      Author: mehmet_dora
 */

#ifndef MODULES_FFT_DSP_H_
#define MODULES_FFT_DSP_H_



#include "arm_math.h"
#include "stdint.h"

#define FFT_SIZE        256 			// Bu sayı kadar ADC verisi üzerinde FFT uygulanacaktır, 2 nin kuvveti olmalı
#define SAMPLE_RATE     200.0f

#define MOTOR_FREQ_THRESHOLD_HZ  8.0f	// Bu değer FFT sonucu ile karşılaştırılacak kontrol değişkeni, test edilerek belirlenmeli
#define POWER_THRESHOLD          500.0f	// Bu değer sinyalin gücü ile karşılaştırılacak kontrol değişkenidir,
										// Sinyalin gücüne göre kontrol edilmesini sağlar, hassasiyet ayarı gibi
										// Yine boş ortamda ve hareket zamanında ölçülerek belirlenmelidir


// Tespit sonucu
typedef enum {
    DETECT_NOTHING,
    DETECT_YAYA,
    DETECT_MOTORSIKLET
} DetectionResult;


typedef struct{
	DetectionResult object_class;
	float dominant_freq_hz;
	float speed_kmh;
	float peak_power;
}DetectionInfo;



extern volatile float debug_moto_power;
extern volatile float debug_yaya_power;


// Dışarıya açık fonksiyon — main.c bu fonksiyonu çağırır
DetectionInfo fft_process(uint16_t* adc_buffer);


#endif /* MODULES_FFT_DSP_H_ */
