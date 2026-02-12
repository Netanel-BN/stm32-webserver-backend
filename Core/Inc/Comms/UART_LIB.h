/*
 * UART_LIB.h
 *
 *  Created on: Feb 12, 2026
 *      Author: Netanel Berihun
 */

#ifndef INC_UART_LIB_H_
#define INC_UART_LIB_H_

#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 128

#include "main.h"

void uart_init(UART_HandleTypeDef *huart);

#endif /* INC_UART_LIB_H_ */
