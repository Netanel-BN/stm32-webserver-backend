/*
 * UART_LIB.c
 *
 *  Created on: Feb 12, 2026
 *      Author: Feuerstein
 */




#include "Comms/UART_LIB.h"
#include <string.h>
#include <stdio.h>

uint8_t rx_buffer[RX_BUFFER_SIZE];    // Main receive buffer
uint8_t tx_buffer[TX_BUFFER_SIZE];    // Transmit buffer
uint16_t rx_index = 0;
volatile uint8_t tx_busy = 0;         // Flag to track if transmission is ongoing
// Debug counters - visible in debugger
volatile uint32_t uart_rx_count = 0;      // Total bytes received
volatile uint32_t uart_tx_count = 0;      // Total bytes sent
volatile uint32_t uart_cmd_count = 0;     // Commands processed
volatile uint32_t uart_error_count = 0;   // Errors detected
volatile uint16_t last_rx_length = 0;     // Last received chunk length


static void subscribe_to_idle(UART_HandleTypeDef *huart) {
	// Start UART reception in interrupt mode - receive up to RX_BUFFER_SIZE bytes
	HAL_UART_Receive_IT(huart, rx_buffer, RX_BUFFER_SIZE);
}

static void process_uart_command(UART_HandleTypeDef *huart,
		uint8_t *buffer, uint16_t length)
{
  char response[64];

  // Null-terminate the received buffer for safe string operations
  if (length < RX_BUFFER_SIZE)
  {
    buffer[length] = '\0';
  }
  else
  {
    buffer[RX_BUFFER_SIZE - 1] = '\0';
  }

  // Update statistics
  uart_rx_count += length;
  last_rx_length = length;

  // Example: Echo the received command
  if (strncmp((char*)buffer, "LED_ON", 6) == 0)
  {
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    sprintf(response, "LED turned ON\r\n");
    uart_cmd_count++;
  }
  else if (strncmp((char*)buffer, "LED_OFF", 7) == 0)
  {
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    sprintf(response, "LED turned OFF\r\n");
    uart_cmd_count++;
  }
  else if (strncmp((char*)buffer, "STATUS", 6) == 0)
  {
    sprintf(response, "System OK - Uptime: %lu ms\r\n", HAL_GetTick());
    uart_cmd_count++;
  }
  else
  {
    // Echo back the received command (safely copy to avoid overflow)
    sprintf(response, "Received: ");
    int prefix_len = strlen(response);
    int copy_len = (length < 50) ? length : 50;  // Leave room for prefix and suffix
    memcpy(response + prefix_len, buffer, copy_len);
    sprintf(response + prefix_len + copy_len, "\r\n");
  }

  // Copy response to transmit buffer
  uint16_t response_len = strlen(response);
  if (response_len > TX_BUFFER_SIZE)
  {
    response_len = TX_BUFFER_SIZE;
  }
  memcpy(tx_buffer, response, response_len);

  // Send response using interrupt-based transmission (non-blocking)
  HAL_StatusTypeDef status = HAL_UART_Transmit_IT(huart, tx_buffer, response_len);

  if (status == HAL_OK)
  {
    tx_busy = 1;
  }
  else
  {
    uart_error_count++;  // Track transmission errors
  }
}

void uart_init(UART_HandleTypeDef *huart) {
	// Enable UART Idle Line Interrupt
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
	subscribe_to_idle(huart);
}

/* Callbacks */

// UART Idle Line Interrupt - handles variable-length messages
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart->Instance == USART1)
  {
    // Toggle LED to show callback is triggered
    HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);

    process_uart_command(huart,rx_buffer, size);

    // Clear buffer before reuse
    memset(rx_buffer, 0, RX_BUFFER_SIZE);

    // Re-subscribe to idle interrupt
    subscribe_to_idle(huart);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart_error_count++;

    // Clear error flags
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);

    // Restart reception
    memset(rx_buffer, 0, RX_BUFFER_SIZE);
    subscribe_to_idle(huart);
  }
}

