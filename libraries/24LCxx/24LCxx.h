/*
 * 24LCxx.h
 *
 *  Created on: May 26, 2024
 *      Author: Selina
 */

#ifndef INC_24LCXX_H_
#define INC_24LCXX_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define EEPROM_WRITE_TIMEOUT 500
#define EEPROM_READ_TIMEOUT  500
#define EEPROM_OK            1
#define EEPROM_ERROR         0
#define EEPROM_PAGE_SIZE     64  // Adjust based on the EEPROM model (24LC16, 24LC256, etc.)

typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t address;		// EEPROM's i2c address
    uint16_t eeprom_size;   // EEPROM size in bytes
    uint16_t page_size;     // EEPROM page size in bytes
}_24LCxx_hardware_t;

typedef struct {
    _24LCxx_hardware_t hardware;

    uint16_t eeprom_bytes;

} eeprom_24LCxx_t;

/*
 * Initialize the EEPROM (I2C port, address, size)
 */
void EEPROM_init(eeprom_24LCxx_t *_eeprom, I2C_HandleTypeDef *i2c, uint8_t address, uint16_t eeprom_size);

/*
 * Function to get EEPROM size
 */
uint16_t EEPROM_Length(eeprom_24LCxx_t *_eeprom);

/*
 * Write 1 byte to EEPROM
 */
uint8_t EEPROM_write_byte(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t data_write);

/*
 * Write multiple bytes to EEPROM
 */
uint8_t EEPROM_write_bytes(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t *data_write, uint16_t data_len);

/*
 * Read 1 byte from EEPROM
 */
uint8_t EEPROM_read_byte(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t *data_write);

/*
 * Read multiple bytes from EEPROM
 */
uint8_t EEPROM_read_bytes(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t *data_write, uint16_t data_len);

/*
 * Clear EEPROM
 */
void clear_EEPROM(eeprom_24LCxx_t *_eeprom);

#endif /* INC_24LCXX_H_ */
