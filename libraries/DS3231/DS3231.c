/*
 * DS3231.c
 *
 *  Created on: Apr 8, 2024
 *      Author: Bui Duong Quoc Bao
 */
#include "DS3231.h"
#define RTC_ADDR (0x68<<1)

// Cach 1: Khai Bao Ham "extern I2C_HandleTypeDef hi2c1"
//extern I2C_HandleTypeDef hi2c1;

// Cach 2: Khai Bao Ham "extern I2C_HandleTypeDef hi2c1"

// static: khi khai bao bien static thi chi cac bien trong file nay moi duoc su dung
static I2C_HandleTypeDef *ds_i2c;

void rtc_init(I2C_HandleTypeDef *i2c)
{
	ds_i2c = i2c;
}

uint8_t Decimal2BCD(uint8_t num)
{
	return (num/10)<<4|(num%10);
}

uint8_t BCD2Decimal(uint8_t num)
{
	return (num>>4)*10+(num&0x0F);
}

void set_currently_timer_ds3231(DS3231_data_t *st)
{
	st->Day	= 5;
	st->Date = 23;
	st->Month = 05;
	st->Year = 2024;
	st-> Hour = 17;
	st-> Minute = 30;
	st-> Second = 0;
}

void DS3231_write_time(DS3231_data_t *dt)
{
	uint8_t data_tran[8];
	data_tran[0] = 0x00;
	data_tran[1] = Decimal2BCD(dt->Second);
	data_tran[2] = Decimal2BCD(dt->Minute);
	data_tran[3] = Decimal2BCD(dt->Hour);
	data_tran[4] = Decimal2BCD(dt->Day);
	data_tran[5] = Decimal2BCD(dt->Date);
	data_tran[6] = Decimal2BCD(dt->Month);
	data_tran[7] = Decimal2BCD(dt->Year);

	HAL_I2C_Master_Transmit(ds_i2c, RTC_ADDR, data_tran, 8, 100);
}

void DS3231_read_time(DS3231_data_t *dt)
{
	uint8_t data_receive[7];
	uint8_t add_reg = 0x00; // Dia Chi Bat Dau Muon Doc
	HAL_I2C_Master_Transmit(ds_i2c, RTC_ADDR, &add_reg, 1, 100);
	HAL_I2C_Master_Receive(ds_i2c, RTC_ADDR, data_receive, 7, 100);
	dt->Second = BCD2Decimal(data_receive[0]);
	dt->Minute = BCD2Decimal(data_receive[1]);
	dt->Hour = BCD2Decimal(data_receive[2]);
	dt->Day = BCD2Decimal(data_receive[3]);
	dt->Date = BCD2Decimal(data_receive[4]);
	dt->Month = BCD2Decimal(data_receive[5]);
	dt->Year = BCD2Decimal(data_receive[6]);
}


//uint8_t rtc_read_temp()//DateTime *dt
//{
//	uint8_t data_temp[2];
//	uint8_t add_reg = 0x11;
//	HAL_I2C_Master_Transmit(ds_i2c, RTC_ADDR, &add_reg, 1, 100);
//	HAL_I2C_Master_Receive(ds_i2c, RTC_ADDR, data_temp, 2, 100);
////	temp = ((uint16_t)data_temp[0]<<2)|(data_temp[1]>>6);
//
//	return data_temp[0];
//}
//

