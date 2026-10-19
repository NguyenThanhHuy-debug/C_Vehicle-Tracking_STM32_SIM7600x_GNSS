#ifndef DAVITEDSENSOR_H_
#define DAVITEDSENSOR_H_
#include <stdint.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart1;

uint16_t uart_available(void);
void uart_init(void);
void processingdata(uint8_t *data_rxresponse, uint8_t length, uint16_t *data_t,uint32_t *data_F, uint32_t *data_N);

#endif
