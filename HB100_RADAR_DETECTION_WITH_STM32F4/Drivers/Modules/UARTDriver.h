/*
 * UARTDriver.h
 *
 *  Created on: 22 Şub 2026
 *      Author: mehmet_dora
 */

#ifndef MODULES_UARTDRIVER_H_
	#define MODULES_UARTDRIVER_H_

	#include <stdint.h>


	extern volatile char* tx_buffer;	// Buffer array-pointer
	extern volatile uint16_t tx_index;	// Buffer için sonraki yazılacak index tutucu
	extern volatile uint16_t tx_len;	// Buffer boyutunu tutucu


	extern volatile uint8_t rx_data;
	extern volatile uint8_t rx_flag;



	// Polling based
	void uart_send_char(uint8_t data);
	void uart_send_string(char* string);
	uint8_t uart_read_char();

	void uart_init(void);

	// interrupt Based
	void uart_send_string_IT(char* data);
	uint8_t uart_read_char_IT(void);


	// HELPER FONKS
	void int2char(int num, char str[]);



#endif /* MODULES_UARTDRIVER_H_ */
