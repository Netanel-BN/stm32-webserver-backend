/*
 * I2C_SLAVE.c
 *
 *  Created on: Feb 11, 2026
 *      Author: Netanel Berihun
 */


#include "Comms/I2C_SLAVE.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define I2C_SLAVE_RX_BUFFER_SIZE 128
#define I2C_SLAVE_TX_BUFFER_SIZE 64

uint8_t i2c_rx_buffer[I2C_SLAVE_RX_BUFFER_SIZE];
uint8_t i2c_tx_buffer[I2C_SLAVE_TX_BUFFER_SIZE];
volatile uint8_t i2c_data_ready = 0;
volatile uint16_t i2c_rx_length = 0;
// Debug counters
volatile uint32_t i2c_rx_count = 0;
volatile uint32_t i2c_tx_count = 0;
volatile uint32_t i2c_error_count = 0;
volatile bool tx_ready = false;


void i2c_slave_init(I2C_HandleTypeDef *hi2c) {
	// Initialize buffers and state
	memset(i2c_rx_buffer, 0, I2C_SLAVE_RX_BUFFER_SIZE);
	memset(i2c_tx_buffer, 0, I2C_SLAVE_TX_BUFFER_SIZE);
	i2c_data_ready = 0;
	i2c_rx_length = 0;
	i2c_rx_count = 0;
	i2c_tx_count = 0;
	i2c_error_count = 0;

	// Enable I2C listen mode to wait for master communication
	HAL_I2C_EnableListen_IT(hi2c);
}


void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c){
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode) {

	if (TransferDirection == I2C_DIRECTION_RECEIVE) {
	    memset(i2c_tx_buffer, 0, sizeof(i2c_tx_buffer));
	    snprintf((char*)i2c_tx_buffer, sizeof(i2c_tx_buffer),
	            "Hello from STM32 I2C Slave! (#%lu)", i2c_tx_count + 1);

        HAL_I2C_Slave_Seq_Transmit_IT(hi2c, i2c_tx_buffer,
                I2C_SLAVE_TX_BUFFER_SIZE - 1, I2C_FIRST_AND_LAST_FRAME);
    }
    else if (TransferDirection == I2C_DIRECTION_TRANSMIT) {
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2c_rx_buffer, I2C_SLAVE_RX_BUFFER_SIZE, I2C_FIRST_AND_LAST_FRAME);
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c){
	//i2c_rx_length = sizeof(i2c_rx_buffer);
	//i2c_data_ready = 1;
	i2c_rx_count++;
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c){
	i2c_tx_count++;
	//prepare_i2c_response();
}


// I2C Error Callback - Re-enable listening on error to recover and keep flow going
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	uint32_t error = HAL_I2C_GetError(hi2c);

	HAL_I2C_DeInit(hi2c);
	HAL_I2C_Init(hi2c);
	HAL_I2C_EnableListen_IT(hi2c);
}


