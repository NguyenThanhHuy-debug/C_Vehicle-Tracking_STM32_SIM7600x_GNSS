/*
 * FuelSensorRS232.h
 *
 *  Created on: Jun 25, 2024
 *      Author: Selina
 */

#ifndef LIBRARIES_FUELSENSORRS232_FUELSENSORRS232_H_
#define LIBRARIES_FUELSENSORRS232_FUELSENSORRS232_H_

#include "main.h"

#define FUEL_SENSOR_BUFFER_LENGTH		11
#define FUEL_SENSOR_TX_PREFIX			0x31
#define FUEL_SENSOR_RX_PREFIX			0x3E


typedef struct{
	UART_HandleTypeDef 	*huart;
	uint8_t 			address;

	uint8_t 			raw_data[FUEL_SENSOR_BUFFER_LENGTH];

	uint8_t				sensor_found;
	uint8_t 			sensor_check;

	uint8_t 			operation_code;
	uint8_t 			temperature;
	uint16_t 			relative_level;
	uint32_t 			frequency_value;
} fuel_sensor_t;

int fuel_sensor_init(fuel_sensor_t *_sensor, UART_HandleTypeDef *huart, uint8_t address);

void fuel_sensor_UART_callback(fuel_sensor_t *_sensor);

HAL_StatusTypeDef send_command_to_fuel_sensor(fuel_sensor_t *_sensor);
#endif /* LIBRARIES_FUELSENSORRS232_FUELSENSORRS232_H_ */
