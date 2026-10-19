/*
 * FuelSensorRS232.c
 *
 *  Created on: Jun 25, 2024
 *      Author: Selina
 */

#include"FuelSensorRS232.h"

uint8_t uart_rx_buffer[FUEL_SENSOR_BUFFER_LENGTH];
//uint8_t sensor_data[FUEL_SENSOR_BUFFER_LENGTH];
int fuel_sensor_init(fuel_sensor_t *_sensor, UART_HandleTypeDef *huart, uint8_t address){
	_sensor->huart = huart;
	_sensor->address = address;

	HAL_UART_Receive_IT(_sensor->huart, (uint8_t*)uart_rx_buffer, 11);
	return 1;
}

// Function to calculate CRC-8
#define CRC8_POLY 0x07
uint8_t calculate_crc8(uint8_t *data, uint8_t length) {
    uint8_t crc = 0;  // Initialize CRC to 0
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];  // XOR the input data with the CRC
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLY;  // Shift left and XOR with polynomial
            }
            else {
                crc <<= 1;  // Just shift left
            }
        }
    }
    return crc;
}

uint8_t old_sensor_check;
HAL_StatusTypeDef send_command_to_fuel_sensor(fuel_sensor_t *_sensor){
	uint8_t data[4];
	data[0] = FUEL_SENSOR_TX_PREFIX;
	data[1] = _sensor->address;
	data[2] = 0x06;
	data[3] = calculate_crc8(data, 3);

	if(old_sensor_check != _sensor->sensor_check){
		old_sensor_check = _sensor->sensor_check;
		_sensor->sensor_found = 1;
	}
	else{
		_sensor->sensor_found = 0;
	}

	return HAL_UART_Transmit(_sensor->huart, (uint8_t*)data, sizeof(data), 1000);
}

void fuel_sensor_UART_callback(fuel_sensor_t *_sensor){

	if(HAL_UART_Receive_IT(_sensor->huart, (uint8_t*)uart_rx_buffer, 11) == HAL_OK){
		int data_addr = -1;
		for(int i = 0; i < FUEL_SENSOR_BUFFER_LENGTH; ++i){
			if(uart_rx_buffer[i] == FUEL_SENSOR_RX_PREFIX){
				data_addr = i;
				break;
			}
		}

		if(data_addr != -1){
			for(int i = 0; i < FUEL_SENSOR_BUFFER_LENGTH; ++i){
				_sensor->raw_data[i] = uart_rx_buffer[data_addr];
				++data_addr;
				if(data_addr > FUEL_SENSOR_BUFFER_LENGTH) data_addr = 0;
			}
			/*
			 * Write checksum code here
			 *
			 * */
			_sensor->operation_code = _sensor->raw_data[2];
			_sensor->temperature = _sensor->raw_data[3];
			_sensor->relative_level = (uint16_t)(_sensor->raw_data[5] << 8) | _sensor->raw_data[4];
			_sensor->frequency_value = (uint32_t)(_sensor->raw_data[9] << 24) | (_sensor->raw_data[8] << 16) | (_sensor->raw_data[7] << 8) | _sensor->raw_data[6];
		}
		_sensor->sensor_found = 1;
		++_sensor->sensor_check;
		if(_sensor->sensor_check > 200) _sensor->sensor_check = 0;
	}

	return;
}
