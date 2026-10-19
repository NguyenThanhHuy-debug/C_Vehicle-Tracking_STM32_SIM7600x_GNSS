/*
 * 24LCxx.c
 *
 *  Created on: May 26, 2024
 *      Author: Selina
 */
#include "24LCxx.h"

/*
 * Initialize the EEPROM (I2C port, address, size)
 */
void EEPROM_init(eeprom_24LCxx_t *_eeprom, I2C_HandleTypeDef *i2c, uint8_t address, uint16_t eeprom_size) {
    _eeprom->hardware.i2c = i2c;

    _eeprom->hardware.address = address << 1;  // Shift for 7-bit addressing

    _eeprom->hardware.eeprom_size = eeprom_size; // Kbits

    _eeprom->eeprom_bytes = (_eeprom->hardware.eeprom_size * 128); //(EEPROM SIZE * 1024)/8

    _eeprom->hardware.page_size = EEPROM_PAGE_SIZE;
}

/*
 * Function to get EEPROM size
 */
uint16_t EEPROM_Length(eeprom_24LCxx_t *_eeprom) {
    return _eeprom->eeprom_bytes - 1;
}

/*
 * HAL I2C write function
 */
uint8_t i2c_write_bytes(eeprom_24LCxx_t *_eeprom, uint16_t data_addr, uint8_t *data, uint16_t data_len) {
    if (HAL_I2C_Mem_Write(_eeprom->hardware.i2c, _eeprom->hardware.address, data_addr, I2C_MEMADD_SIZE_16BIT, data, data_len, EEPROM_WRITE_TIMEOUT) != HAL_OK) {
        return EEPROM_ERROR;
    }
    HAL_Delay(5);  // EEPROM write delay to ensure the data is written
    return EEPROM_OK;
}

/*
 * Write 1 byte to EEPROM
 */
uint8_t EEPROM_write_byte(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t data_write) {
    return i2c_write_bytes(_eeprom, start_data_addr, &data_write, 1);
}

/*
 * Write multiple bytes to EEPROM
 */
uint8_t EEPROM_write_bytes(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t *data_write, uint16_t data_len) {
    uint16_t page_remaining = _eeprom->hardware.page_size - (start_data_addr % _eeprom->hardware.page_size);
    uint16_t bytes_to_write = (data_len < page_remaining) ? data_len : page_remaining;

    while (data_len > 0) {
        if (i2c_write_bytes(_eeprom, start_data_addr, data_write, bytes_to_write) != EEPROM_OK) {
            return EEPROM_ERROR;
        }
        start_data_addr += bytes_to_write;
        data_write += bytes_to_write;
        data_len -= bytes_to_write;
        bytes_to_write = (data_len < _eeprom->hardware.page_size) ? data_len : _eeprom->hardware.page_size;
    }

    return EEPROM_OK;
}

/*
 * HAL I2C read function
 */
uint8_t i2c_read_bytes(eeprom_24LCxx_t *_eeprom, uint16_t data_addr, uint8_t *data, uint16_t data_len) {
    if (HAL_I2C_Mem_Read(_eeprom->hardware.i2c, _eeprom->hardware.address, data_addr, I2C_MEMADD_SIZE_16BIT, data, data_len, EEPROM_READ_TIMEOUT) != HAL_OK) {
        return EEPROM_ERROR;
    }
    return EEPROM_OK;
}

/*
 * Read 1 byte from EEPROM
 */
uint8_t EEPROM_read_byte(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t *data_write) {
    return i2c_read_bytes(_eeprom, start_data_addr, data_write, 1);
}

/*
 * Read multiple bytes from EEPROM
 */
uint8_t EEPROM_read_bytes(eeprom_24LCxx_t *_eeprom, uint16_t start_data_addr, uint8_t *data_write, uint16_t data_len) {
    return i2c_read_bytes(_eeprom, start_data_addr, data_write, data_len);
}

/*
 * Clear EEPROM
 */
//void clear_EEPROM(eeprom_24LCxx_t *_eeprom) {
//    uint8_t clear_data[_eeprom->hardware.page_size];
//    memset(clear_data, 0xFF, _eeprom->hardware.page_size);  // Fill with 0xFF to "erase"
//
//    for (uint16_t addr = 0; addr < _eeprom->hardware.eeprom_size; addr += _eeprom->hardware.page_size) {
//        EEPROM_write_bytes(_eeprom, addr, clear_data, _eeprom->hardware.page_size);
//    }
//}

void clear_EEPROM(eeprom_24LCxx_t *eeprom) {
	uint16_t eeprom_size = EEPROM_Length(eeprom);  // Get the size of EEPROM

    uint8_t *zeroBlock = (uint8_t *)malloc(eeprom_size * sizeof(uint8_t));  // Allocate memory

    if (zeroBlock == NULL) {
        printf("Memory allocation failed!\r\n");
        return;  // Handle allocation failure
    }

    // Initialize the allocated memory to 0
    memset(zeroBlock, 0, eeprom_size);

    // Write the zeroed buffer to the entire EEPROM
    EEPROM_write_bytes(eeprom, 0, zeroBlock, eeprom_size);

    free(zeroBlock);  // Don't forget to free the allocated memory after use
}






