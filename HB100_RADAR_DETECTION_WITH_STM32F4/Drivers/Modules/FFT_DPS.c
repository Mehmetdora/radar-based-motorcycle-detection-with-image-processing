/*
 * FFT_DPS.c
 *
 *  Created on: May 17, 2026
 *      Author: mehmet_dora
 */


#include "FFT_DSP.h"
#include "RadarSignalHelper.h"
#include "arm_math.h"

static float32_t fft_input[FFT_SIZE];
static float32_t fft_output[FFT_SIZE];
static float32_t fft_mag[FFT_SIZE / 2];

static arm_rfft_fast_instance_f32 fft_instance;		// FFT için özel struct
static uint8_t fft_is_ready = 0;					// ilk init kontrolü için




DetectionInfo fft_process(uint16_t *adc_buffer)
{
    if (!fft_is_ready) {
        arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);	// FFT struct init
        fft_is_ready = 1;
    }


    DetectionInfo info;


    /*
     * Burada mean(ortalama) ADC değeri bulunur, böylece FFT bu ortalama değeri
     * baz alarak asıl değişimleri analiz eder. Bu işlem her buffer dolması sonrası
     * yapılır , böylece sabit bir ortalama değer yerine veriye göre güncellenen ortalama
     * değer bulunur.
     */
    float32_t mean = 0.0f;

    // 1) Ortalama/DC değeri bul
    for (int i = 0; i < FFT_SIZE; i++) {
        mean += adc_buffer[i];
    }
    mean /= FFT_SIZE;





    /*
     * ADC den gelen buffer daki raw verilerden bu mean değeri çıkarılarak asıl değer
     * değişimleri elde edilmiş olunur. Bu yeni değerler başka bir diziye kopyalanır.
     */
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input[i] = (float32_t)adc_buffer[i] - mean;
    }





    // FFT ilgili diziye uygulanır ve sonuç fft_output dizisine kaydedilir
    // sonraki 0 değeri zamansal veriden frekans değerine geçileceğini bildirir
    arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);



    /*
     * FFT sonucu değerler karmaşık sayılardır, bu şekilde düzeltilir
     * Sonrasında bu değerler arasında karşılaştırma yapılabilir.
     */
    /*
    for (int k = 1; k < FFT_SIZE / 2; k++) {
        float32_t real = fft_output[2 * k];
        float32_t imag = fft_output[2 * k + 1];

        fft_mag[k] = sqrtf(real * real + imag * imag);
    }
    */
    arm_cmplx_mag_f32(fft_output, fft_mag, FFT_SIZE / 2);


    // İlk değer ortalamayı temsil eder, bu nedenle ortalama 0 yapılır, 0 etrafında
    // veriler değişecektir
    fft_mag[0] = 0.0f;




    // En baskın(büyük) frekansın bulunması
    float32_t max_mag = 0.0f;
    int max_bin = 0;		// bulunan frekansın indexi



    for (int k = 1; k < FFT_SIZE / 2; k++) {

        float32_t freq = ((float32_t)k * SAMPLE_RATE) / FFT_SIZE;

        // Çok düşük frekans: DC drift, elin yavaş hareketi, op-amp salınımı
        if (freq < MIN_VALID_FREQ_HZ)
            continue;

        // 50 Hz ve çevresi: elektriksel gürültü / leakage
        if (freq > MAINS_NOISE_LOW_HZ && freq < MAINS_NOISE_HIGH_HZ)
            continue;

        // Nyquist'e çok yaklaşma
        if (freq > MAX_VALID_FREQ_HZ)
            continue;

        if (fft_mag[k] > max_mag) {
            max_mag = fft_mag[k];
            max_bin = k;
        }
    }




    /*
     * FFT sonucunda en baskın sinyalin buffer içindeki indexi elde edilir, bu değerden
     * Hz bilgisini öğrenebilmek için örnekleme hızı olan sample rate kullanılarak bulunur.
     */
    float32_t dominant_freq = ((float32_t)max_bin * SAMPLE_RATE) / FFT_SIZE;


    info.dominant_freq_hz = dominant_freq;
    info.peak_power = max_mag;
    info.speed_kmh = radar_calculate_speed_kmh(dominant_freq);




    // Bulunan Hz ve güç değerleri helper içinde sınıflandırılır.
    info.object_class = radar_classify_signal(dominant_freq, max_mag);
    return info;
}
