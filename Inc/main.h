/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include<stdio.h>
#include <inttypes.h>

/*
 * Firmware Version
 * */

#include "ota.h"

#define Version										"DCA.2024.10.08_OTA"
//#define OTA_FIRMWARE_URL							"http://app.iotvision.vn/assets/UpdateFirmware/FirmwareDeoCa.bin"
#define OTA_FIRMWARE_URL							"https://hav.assfa.net/firmware/FirmwareDeoCa.bin"
#define OTA_FIRMWARE_URL_BACKUP						"https://hav.assfa.net/firmware/FirmwareDeoCa.bin"

#define API_GENERATE_PREFIX							"DCA"
#define API_GENERATE_SERI_NUMBER					"https://hav.assfa.net/sn_gen.php?prefix="

//"https://hav.assfa.net/firmware/firmware.bin"


/*
 * API for connect with server
 * */
// API of IoTVision's server
#define API_GET_CODE_IOTVISION						"http://App.IoTVision.vn/api/DCa_DuLieuGuiXuongBoard?CheDo=1&key="
#define API_POST_CODE_IOTVISION						"http://App.IoTVision.vn/api/DCa_DuLieuGuiXuongBoard"

#define API_POST_SEC								"http://App.IoTVision.vn/api/BoardSEC_DuLieu"

#define API_POST_DATA_IOTVISION 					"http://App.IoTVision.vn/api/DCa_DuLieu"

#define API_GET_TIME_IOTVISION						"http://App.IoTVision.vn/api/ThoiGian"

#define API_POST_FIRMWARE_VERSION_IOTVISION			"http://app.iotvision.vn/api/DCa_PhienBan?ID="

//#define FIRMWARE_URL								"https://hav.assfa.net/firmware/firmware.bin"

// API of DeoCa's server
#define API_GET_CODE_DEOCA							"https://k8s.deoca.vn/beta/v2/lora/DCa_DuLieuGuiXuongBoard?CheDo=1&key="
#define API_POST_CODE_DEOCA							"https://k8s.deoca.vn/beta/v2/lora/DCa_DuLieu"

#define API_POST_DATA_DEOCA 						"https://k8s.deoca.vn/beta/v2/lora/raw"

/*
 * SD card's path
 * */
#define BACKUP_PATH									"/BACKUP"
#define TEMP_PATH									"/TEMP"
#define FIRMWARE_PATH								"/FIRMWARE"

/*
 * Define
 * */
#define RUN_SIM
#define RUN_GPS
//#define RUN_LigoSENSOR

#define USE_DaviteqSensor
//#define USE_LigoSensor

//#define USE_ACC

#ifdef USE_LigoSensor
#include "../libraries/FuelSensorRS232/FuelSensorRS232.h"
#endif

#ifdef USE_DaviteqSensor
#include "../libraries/DaviteqSensor/davitedSensor.h"
#define FRAM_SIZE 22
#endif

#ifdef RUN_SIM
#include "../libraries/sim7600/sim7600.h"
#endif

#ifdef RUN_GPS
#include "../libraries/TAU1201/TAU1201.h"
#endif

#include "../libraries/SHT31/SHT31.h"
#include "../libraries/DS3231/DS3231.h"

#include "../libraries/24LCxx/24LCxx.h"
#define EEPROM_I2C_ADDRESS              			0x50
#define EEPROM_SIZE                     			256

#include "timer_flag.h"

typedef struct{
	int Day;
	int Date;
	int Month;
	int Year;
	int Hour;
	int Minute;
	int Second;
} time_t;

typedef struct{
	int CodeGet;

	int HTTPGetCode;
	int HTTPPostCode;
	char HTTPGetResponse[256];
	char HTTPPostResponse[256];

	time_t rtc;
}server_t;

typedef struct{
	int date;
	int month;
	int year;
} install_time_t;

typedef struct{
	uint8_t update_status;
//	Board's ID
	char ID[15];

//	Thông tin tài xế (Mã NFC)
	char driverID[20];
//	Trạng thái đăng nhập, đăng xuất
	uint8_t check_in_out_state;

//	3 ngưỡng giới hạn tốc độ
	uint16_t speed_limit_1;
	uint16_t speed_limit_2;
	uint16_t speed_limit_3;

//  Ngày thiết lập board
	install_time_t install_time;
//	Last Latitude
	float LastLatitude;
//	Last LongiTude
	float LastLongitude;
//	Cờ lệnh OTA
	uint8_t OTA_Flag;

}board_parameter_t;



/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ACC_Pin GPIO_PIN_2
#define ACC_GPIO_Port GPIOC
#define ACC_EXTI_IRQn EXTI2_IRQn
#define SD_CS_Pin GPIO_PIN_4
#define SD_CS_GPIO_Port GPIOA
#define SD_CD_Pin GPIO_PIN_4
#define SD_CD_GPIO_Port GPIOC
#define SD_CD_EXTI_IRQn EXTI4_IRQn
#define SIM_RST_Pin GPIO_PIN_5
#define SIM_RST_GPIO_Port GPIOC
#define SIM_STATUS_Pin GPIO_PIN_0
#define SIM_STATUS_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_1
#define BUZZER_GPIO_Port GPIOB
#define SIM_HRST_Pin GPIO_PIN_8
#define SIM_HRST_GPIO_Port GPIOE
#define SIM_PWR_Pin GPIO_PIN_15
#define SIM_PWR_GPIO_Port GPIOA
#define LED_PWR_Pin GPIO_PIN_3
#define LED_PWR_GPIO_Port GPIOD
#define LED_SENSOR_Pin GPIO_PIN_4
#define LED_SENSOR_GPIO_Port GPIOD
#define LED_DRIVER_Pin GPIO_PIN_5
#define LED_DRIVER_GPIO_Port GPIOD
#define LED_SERVER_Pin GPIO_PIN_6
#define LED_SERVER_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
