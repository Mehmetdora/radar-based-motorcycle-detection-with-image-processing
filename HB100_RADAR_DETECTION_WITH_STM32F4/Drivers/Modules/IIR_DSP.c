/*
 * IIR_DSP.c
 *
 *  Created on: May 8, 2026
 *      Author: mehmet_dora
 */

#include "IIR_DSP.h"
#include "arm_math.h"
#include <stdint.h>

#define NUM_STAGES 1



arm_biquad_casd_df1_inst_f32 iir_filter;


static float32_t iir_state[4 * NUM_STAGES];  // State buffer (4 float per stage)

// ==================== Filtre Katsayıları ====================
// 100 Hz Sampling Rate, 10 Hz kesme frekansı, Butterworth
// Aşağıdaki katsayılar MATLAB'da designfilt() ile oluşturulabilir
// ya da online tools kullanılabilir

static float32_t iir_coefficients[5 * NUM_STAGES] = {
    // b0, b1, b2, a1, a2 (a0 = 1 olduğu için yazılmaz)
    0.1989f,  -0.3978f,  0.1989f,  -1.1430f,  0.4128f  // 10 Hz LPF
};





void IIR_Filter_Init(void) {
    // CMSIS-DSP IIR Filtresi Başlat
    arm_biquad_cascade_df1_init_f32(
        &iir_filter,           // Filter instance
        NUM_STAGES,            // Kaç tane 2. derece filtre
        iir_coefficients,      // Filtre katsayıları
        iir_state              // State buffer
    );
}

void FirstOrderIIR_Init (FirstOrderIIR *filt, float alpha){

	if(alpha < 0.0f){
		filt->alpha = 0.0f;
	}else if(alpha > 1.0f){
		filt->alpha = 1.0f;
	}else{
		filt->alpha = alpha;
	}

	filt->out = 0.0f;

}


float FirstOrderIIR_Update (FirstOrderIIR *filt, uint16_t in){


	//float32_t adc_normalized = (float32_t)in / 4095.0f;

	// CMSIS-DSP IIR Filtresi Uygula

	/*
	arm_biquad_cascade_df1_f32(
		&iir_filter,           // Filter instance
		&adc_normalized,       // Giriş: 1 örnek
		&filt->out,  // Çıkış: filtreli değer
		1                      // Örnek sayısı
	);

	*/

	filt->out = (filt->alpha * in) + ((1 - filt->alpha) * filt->out);

	return filt->out;
}









