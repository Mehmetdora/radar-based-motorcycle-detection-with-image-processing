/*
 * RadarSignalHelper.h
 *
 *  Created on: May 31, 2026
 *      Author: mehmet_dora
 */

#ifndef MODULES_RADARSIGNALHELPER_H_
#define MODULES_RADARSIGNALHELPER_H_

#include <stdint.h>
#include "FFT_DSP.h"

#define MOTOR_CONFIRM_COUNT 3

#define RADAR_HB100_HZ_PER_KMH       19.49f
#define RADAR_UART_MESSAGE_MAX_LEN   96U

typedef struct {
	DetectionResult object_class;
	float peak_frequency_hz;
	float signal_power;
	float estimated_speed_kmh;
	uint8_t motion_detected;
} RadarSignalReport;

float radar_calculate_speed_kmh(float peak_frequency_hz);
DetectionResult radar_classify_signal(float peak_frequency_hz, float signal_power);
RadarSignalReport radar_create_report(float peak_frequency_hz, float signal_power);
RadarSignalReport radar_create_report_from_detection(const DetectionInfo *detection);
uint16_t radar_format_uart_message(const RadarSignalReport *report, char *buffer, uint16_t buffer_size);
uint16_t radar_format_final_uart_message(const RadarSignalReport *report, char *buffer, uint16_t buffer_size, uint8_t confirm_count);
const char* radar_detection_to_text(DetectionResult result);

#endif /* MODULES_RADARSIGNALHELPER_H_ */
