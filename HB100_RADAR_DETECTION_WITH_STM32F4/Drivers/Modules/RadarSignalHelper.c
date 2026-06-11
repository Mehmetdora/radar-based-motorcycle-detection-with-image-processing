/*
 * RadarSignalHelper.c
 *
 *  Created on: May 31, 2026
 *      Author: mehmet_dora
 */

#include "RadarSignalHelper.h"

#define RADAR_DECIMAL_SCALE_2  100



static void append_char(char **cursor, uint16_t *remaining, char value)
{
	if (*remaining <= 1U) {
		return;
	}

	**cursor = value;
	(*cursor)++;
	(*remaining)--;
	**cursor = '\0';
}

static void append_string(char **cursor, uint16_t *remaining, const char *value)
{
	while (*value != '\0') {
		append_char(cursor, remaining, *value);
		value++;
	}
}

static void append_uint(char **cursor, uint16_t *remaining, uint32_t value)
{
	char temp[10];
	uint8_t index = 0;

	if (value == 0U) {
		append_char(cursor, remaining, '0');
		return;
	}

	while ((value > 0U) && (index < sizeof(temp))) {
		temp[index] = (char)('0' + (value % 10U));
		value /= 10U;
		index++;
	}

	while (index > 0U) {
		index--;
		append_char(cursor, remaining, temp[index]);
	}
}

static uint32_t float_to_scaled_positive(float value, uint32_t scale)
{
	if (value <= 0.0f) {
		return 0U;
	}

	return (uint32_t)((value * (float)scale) + 0.5f);
}

static void append_fixed_2(char **cursor, uint16_t *remaining, float value)
{
	uint32_t scaled_value;
	uint32_t whole_part;
	uint32_t fraction_part;

	if (value < 0.0f) {
		append_char(cursor, remaining, '-');
		value = -value;
	}

	scaled_value = float_to_scaled_positive(value, RADAR_DECIMAL_SCALE_2);
	whole_part = scaled_value / RADAR_DECIMAL_SCALE_2;
	fraction_part = scaled_value % RADAR_DECIMAL_SCALE_2;

	append_uint(cursor, remaining, whole_part);
	append_char(cursor, remaining, '.');

	if (fraction_part < 10U) {
		append_char(cursor, remaining, '0');
	}
	append_uint(cursor, remaining, fraction_part);
}

float radar_calculate_speed_kmh(float peak_frequency_hz)
{
	if (peak_frequency_hz <= 0.0f) {
		return 0.0f;
	}

	return peak_frequency_hz / RADAR_HB100_HZ_PER_KMH;
}

DetectionResult radar_classify_signal(float peak_frequency_hz, float signal_power)
{
    if (signal_power < POWER_THRESHOLD) {
        return DETECT_NOTHING;
    }

    if (peak_frequency_hz < MIN_VALID_FREQ_HZ) {
        return DETECT_NOTHING;
    }

    if (peak_frequency_hz >= MOTOR_FREQ_THRESHOLD_HZ) {
        return DETECT_MOTORSIKLET;
    }

    return DETECT_YAYA;
}




RadarSignalReport radar_create_report(float peak_frequency_hz, float signal_power)
{
	RadarSignalReport report;

	report.peak_frequency_hz = peak_frequency_hz;
	report.signal_power = signal_power;
	report.estimated_speed_kmh = radar_calculate_speed_kmh(peak_frequency_hz);
	report.object_class = radar_classify_signal(peak_frequency_hz, signal_power);
	report.motion_detected = (report.object_class != DETECT_NOTHING) ? 1U : 0U;

	return report;
}

RadarSignalReport radar_create_report_from_detection(const DetectionInfo *detection)
{
	if (detection == 0) {
		return radar_create_report(0.0f, 0.0f);
	}

	return radar_create_report(detection->dominant_freq_hz, detection->peak_power);
}

const char* radar_detection_to_text(DetectionResult result)
{
	switch (result) {
	case DETECT_MOTORSIKLET:
		return "MOTOR";
	case DETECT_YAYA:
		return "YAYA";
	case DETECT_NOTHING:
	default:
		return "NONE";
	}
}

uint16_t radar_format_uart_message(const RadarSignalReport *report, char *buffer, uint16_t buffer_size)
{
	char *cursor = buffer;
	uint16_t remaining = buffer_size;

	if ((report == 0) || (buffer == 0) || (buffer_size == 0U)) {
		return 0U;
	}

	buffer[0] = '\0';

	append_string(&cursor, &remaining, "OBJ=");
	append_string(&cursor, &remaining, radar_detection_to_text(report->object_class));
	append_string(&cursor, &remaining, ",MOTION=");
	append_uint(&cursor, &remaining, report->motion_detected);
	append_string(&cursor, &remaining, ",POWER=");
	append_fixed_2(&cursor, &remaining, report->signal_power);
	append_string(&cursor, &remaining, ",FREQ_HZ=");
	append_fixed_2(&cursor, &remaining, report->peak_frequency_hz);
	append_string(&cursor, &remaining, ",SPEED_KMH=");
	append_fixed_2(&cursor, &remaining, report->estimated_speed_kmh);
	append_string(&cursor, &remaining, "\r\n");

	return (uint16_t)(cursor - buffer);
}





uint16_t radar_format_final_uart_message(const RadarSignalReport *report, char *buffer, uint16_t buffer_size, uint8_t confirm_count)
{
	char *cursor = buffer;
	uint16_t remaining = buffer_size;

	if ((report == 0) || (buffer == 0) || (buffer_size == 0U)) {
		return 0U;
	}

	buffer[0] = '\0';
	append_string(&cursor, &remaining, "FINAL=TRUE");
	append_string(&cursor, &remaining, ",CONFIRM_COUNT=");
	append_uint(&cursor, &remaining, confirm_count);
	append_string(&cursor, &remaining, ",OBJ=");
	append_string(&cursor, &remaining, radar_detection_to_text(report->object_class));
	append_string(&cursor, &remaining, ",MOTION=");
	append_uint(&cursor, &remaining, report->motion_detected);
	append_string(&cursor, &remaining, ",POWER=");
	append_fixed_2(&cursor, &remaining, report->signal_power);
	append_string(&cursor, &remaining, ",FREQ_HZ=");
	append_fixed_2(&cursor, &remaining, report->peak_frequency_hz);
	append_string(&cursor, &remaining, ",SPEED_KMH=");
	append_fixed_2(&cursor, &remaining, report->estimated_speed_kmh);
	append_string(&cursor, &remaining, "\r\n");

	return (uint16_t)(cursor - buffer);
}
