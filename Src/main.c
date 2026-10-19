 /* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include"sd.h"
sd_card_t sd;


#ifdef RUN_SIM
sim7600_t sim;
#endif

#ifdef RUN_GPS
TAU1201_t gps;
#endif

flagType _Flag;

DS3231_data_t date_time;

eeprom_24LCxx_t eeprom;

uint8_t buzzer_used;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */
float temperature = 0;
float humidity = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_UART4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t ACC_State;
float Latitude, Longitude, Speed, Satelites;

int ServerConnected, ServerConnect_IoTVision, ServerConnect_DeoCa;

char debugStr[100];


#ifdef USE_LigoSensor
fuel_sensor_t fuel_sensor;
#endif

#ifdef USE_DaviteqSensor
uint8_t data_rxResponse[FRAM_SIZE];
uint16_t data_t;
uint32_t data_F, data_N;
#endif


server_t _Server;
board_parameter_t BoardParameter;

#define UNIQUE_ID_BASE_ADDR  (0x1FFF7A10U)  // Địa chỉ cơ sở của thanh ghi chứa Unique Device ID cho STM32F4
char uniqueID_str[24] = {0};  				// Mảng để lưu chuỗi Unique ID sau khi chuyển đổi
/**
 * @brief Đọc mã định danh duy nhất (Unique Device ID) từ các thanh ghi của STM32
 * @param uniqueID_str: Con trỏ đến chuỗi để lưu Unique ID ở định dạng chuỗi ký tự
 */
void STM32_GetUniqueID(char *uniqueID_str) {
	  // Khai báo mảng để lưu giá trị Unique ID dạng 3 thanh ghi 32-bit
    uint32_t unique_id[3];
    // Đọc 32-bit thấp (Least Significant Bits - LSB)
    unique_id[0] = *(uint32_t*)(UNIQUE_ID_BASE_ADDR);        // Lower 32 bits
    // Đọc 32-bit giữa (Middle Significant Bits - MSB)
    unique_id[1] = *(uint32_t*)(UNIQUE_ID_BASE_ADDR + 4U);   // Middle 32 bits
    // Đọc 32-bit cao (Most Significant Bits - MSB)
    unique_id[2] = *(uint32_t*)(UNIQUE_ID_BASE_ADDR + 8U);   // Upper 32 bits
    // Chuyển đổi 3 giá trị 32-bit sang chuỗi ký tự dạng HEX (in hoa)
    // Chuỗi được định dạng: MSB (8 ký tự) + MSB (8 ký tự) + LSB (8 ký tự)
    sprintf(uniqueID_str, "%08" PRIX32 "%08" PRIX32 "%08" PRIX32, unique_id[2], unique_id[1], unique_id[0]);
}

/*
 * Bip tone for buzzer
 * Delay is a time
 * */
int Bip(uint32_t delay){
	buzzer_used = 1;
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
	HAL_Delay(delay);
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
	HAL_Delay(delay);
	buzzer_used = 0;
	return 1;
}


/*
 * UART interupt function
 * */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
// LIGO sensor Interrupt
#ifdef USE_LigoSensor
	if(huart == fuel_sensor.huart){
		fuel_sensor_UART_callback(&fuel_sensor);
	}
#endif

#ifdef USE_DaviteqSensor
	if(huart == &huart1){
		processingdata(data_rxResponse, FRAM_SIZE, &data_t,&data_F, &data_N);
		printf("Fuel sensor: %d | %d | %d\r\n", (int)data_t, (int)data_F, (int)data_N);
		HAL_UART_Receive_IT(&huart1, data_rxResponse, 22);

	}
#endif

// GPS Interrupt
#ifdef RUN_GPS
	if(huart == gps.huart){
		GPS_UART_CallBack(&gps);
	}
#endif
}

/*
 * UART DMA Callback function
 * */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
#ifdef RUN_SIM
	SIM_UART_DMA_CallBack(&sim);
#endif
}

uint8_t sd_card_found;
/*
 * GPIO Interrupt function
 * */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == ACC_Pin){
		ACC_State = HAL_GPIO_ReadPin(ACC_GPIO_Port, ACC_Pin);
	}

	if(GPIO_Pin == SD_CD_Pin){
		if(HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == 0){
			sd.card_detect = 1;
		}
		else{
			sd.card_detect = 0;
			if(sd.card_mouted == 1){
				card_unmount(&sd);
			}
		}
	}
}

int countSpeed, Sounded, countSounded;

int sim_stt;
/*
 * Timer Interrupt function
 * */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
//	Interrupt 50ms
	if(htim->Instance == TIM2){
		Flag_CallBack(&_Flag);
		sim_stt = HAL_GPIO_ReadPin(SIM_STATUS_GPIO_Port, SIM_STATUS_Pin);
	}
//	Interrupt 250ms
	if(htim->Instance == TIM3){
		if(BoardParameter.speed_limit_1 > 0){
			if(Speed < BoardParameter.speed_limit_1){
				if(!buzzer_used){
					HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
				}
				Sounded = 0;
//				if(count)
				countSounded = 0;
			}
			else if((Speed >= BoardParameter.speed_limit_1) && (Speed < BoardParameter.speed_limit_2)){
				if(!Sounded){
					if(countSpeed == 0){
						if(!buzzer_used){
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
						}
						countSpeed = -1;
					}
					if(countSounded == 0){
						HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
						++countSounded;
					}
					else if(countSounded == 1){
						HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
						Sounded = 1;
						countSounded = 0;
					}
				}
				else{
					if(!buzzer_used){
						HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
					}
				}
			}
			else if((Speed >= BoardParameter.speed_limit_2) && (Speed < BoardParameter.speed_limit_3)){
				if((countSpeed == 3) && (!HAL_GPIO_ReadPin(BUZZER_GPIO_Port, BUZZER_Pin))){
					HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
					countSpeed = 0;
				}
				else{
					HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
					++countSpeed;
				}
			}
			else if(Speed >= BoardParameter.speed_limit_3){
				HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
				countSpeed = 0;
			}
		}
		if(ServerConnected == 1){
			HAL_GPIO_TogglePin(LED_SERVER_GPIO_Port, LED_SERVER_Pin);
		}
		else{
			HAL_GPIO_WritePin(LED_SERVER_GPIO_Port, LED_SERVER_Pin, GPIO_PIN_RESET);
		}
	}
	//	Interrupt 500ms
	if(htim->Instance == TIM4){
		if(BoardParameter.check_in_out_state){
			HAL_GPIO_TogglePin(LED_DRIVER_GPIO_Port, LED_DRIVER_Pin);
		}
		else{
			HAL_GPIO_WritePin(LED_DRIVER_GPIO_Port, LED_DRIVER_Pin, GPIO_PIN_RESET);
		}

		if(sd.card_mouted == 1){
			HAL_GPIO_WritePin(LED_SENSOR_GPIO_Port, LED_SENSOR_Pin, GPIO_PIN_SET);
		}
		else{
			HAL_GPIO_WritePin(LED_SENSOR_GPIO_Port, LED_SENSOR_Pin, GPIO_PIN_RESET);
		}
	}
}

char DebugGPSLocation[200];
/*
 * Check if the file opened is new or old
 * If new: write a header for this file
 * If old: do nothing
 * */
void check_new_file(){
	if(file_size() == 0){
		file_write(&sd, "Time,Latitude,Longitude,Speed(km/h),Satelite,ACC,NFC ID,Fuel,Fuel Temp(*C),Temperature(*C),Humidity(%),Location link\n");
	}
}

char folderPath[40];
char fileName[20];
char dataWrite[200];
/*
 * Function to write log to SD card
 * */
void LogData(){
	DS3231_read_time(&date_time);
	if(date_time.Month < 10){
		sprintf(folderPath, "%s/%d_0%d", BACKUP_PATH, (date_time.Year + 2000), date_time.Month);
	}
	else sprintf(folderPath, "%s/%d_%d", BACKUP_PATH, (date_time.Year + 2000), date_time.Month);
//	sprintf(folderPath, "%s/2024_10", BACKUP_PATH);
	//
//	sprintf(folderPath, "%s", TEMP_PATH);
//	sprintf(fileName,"%d_%d_%d.csv", (date_time.Year + 2000), date_time.Month, date_time.Date);

	sprintf(fileName,"%d.csv", date_time.Date);

	if(file_open(&sd, folderPath, fileName, FA_WRITE | FA_OPEN_ALWAYS) == FR_OK){
		check_new_file();

		sprintf(dataWrite,"%d:%d:%d,%f,%f,%.2f,%f,%d,%s,%d,%d,NS,NS,\"https://www.google.com/maps/?q=%f,%f\"\n",
				date_time.Hour,
				date_time.Minute,
				date_time.Second,
				Latitude,
				Longitude,
				Speed,
				Satelites,
				ACC_State,
				BoardParameter.driverID,
#ifdef USE_LigoSensor
				(fuel_sensor.sensor_found)? fuel_sensor.relative_level: -1,
				(fuel_sensor.sensor_found)? fuel_sensor.temperature: -1,
#endif

#ifdef USE_DaviteqSensor
				(int) data_F,
				(int) data_N,
#endif
				Latitude,
				Longitude);
		file_write(&sd, dataWrite);
	}

	file_close(&sd);
}

void GetCodeFromServer();
/*
 *
 * Hàm truy�?n data v�? server
 *
 * */
#define POST_SERVER_IOTVISION
#define POST_SERVER_DEOCA

char DataToPost[200];
void PostDataToServer(){
	printf("Post data to servers\r\n");

#ifdef POST_SERVER_IOTVISION
	memset(DataToPost, 0, sizeof(DataToPost));
	DS3231_read_time(&date_time);

	//int count = 0;
//	while(strlen(sim.PhoneNumber) < 10){
//		GetPhoneNumber(&sim);
//		++count;
//		if(count > 10){
//			printf("SIM number error\r\n");
//			return;
//		}
//	}

	sprintf(DataToPost,"{\"ID\": \"%s\",\"S\": \"%f;%f;%.1f;%d;0;0;%d;%d;%d;%d%d;%d:%d:%d %d/%d/%d;%s;1;%s;%ld/%ld\"}",
			BoardParameter.ID, // ID của board
			Longitude, //S0: Kinh độ
			Latitude, //S1: Vĩ độ
			Speed,
			(Satelites < 4)? -1: ACC_State,
#ifdef USE_LigoSensor
			(fuel_sensor.sensor_found)? fuel_sensor.relative_level: -1,
#endif
#ifdef USE_DaviteqSensor
			(int) data_F,
#endif
//			gps.speed_km,
//			gps.speed_k,
			-1,
			-1,
			sim.Signal,
			sim.NetTech,
			date_time.Hour,
			date_time.Minute,
			date_time.Second,
			date_time.Date,
			date_time.Month,
			(date_time.Year + 2000),
			BoardParameter.driverID,
			sim.PhoneNumber,
			(sd.total_sectors_KiB - sd.free_sectors_KiB),
			sd.total_sectors_KiB);

	LogData();
	if((Longitude != 0) && (Latitude != 0)){
		if(http_post(&sim, API_POST_DATA_IOTVISION, DataToPost) == SIM7600_OK){
			ServerConnect_IoTVision = 1;
			printf("Post data (%s) to IoTVision's server completed\r\n", DataToPost);
		}
		else{
			ServerConnect_IoTVision = 0;
			printf("Post data to IoTVision's server fail, saving data into SD card and send to server later\r\n");

			// Save data cannot send to server into SD card
//			if(file_open(&sd, TEMP_PATH, "1.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK){
//				file_write(&sd, DataToPost);
//				file_close(&sd);
//			}

		}
		HAL_Delay(1000);
	}

#endif

GetCodeFromServer();

//	https://hav.assfa.net/postdata.php?device=Device1&lat=10.854978&long=106.768051&tech=lte&mobile=100&wifi=100&bat=4
//	char DataToPostHAV[200];
//	sprintf(DataToPostHAV,"https://hav.assfa.net/postdata.php?device=%s&lat=%f&long=%f&tech=lte&mobile=%d&wifi=100&bat=4", BoardParameter.ID, Latitude, Longitude, sim.Signal);
//	http_get(&sim,DataToPostHAV,  _Server.HTTPGetResponse, &_Server.HTTPGetCode);

#ifdef POST_SERVER_DEOCA
	memset(DataToPost, 0, sizeof(DataToPost));
	DS3231_read_time(&date_time);
	sprintf(DataToPost,"{\"ID\": \"%s\",\"S\": \"%f;%f;%.1f;%d;0;0;%d;%d;%d;%d%d;%d:%d:%d %d/%d/%d;%s;1;%s;%ld/%ld\"}",
			BoardParameter.ID, // ID của board
			Latitude, //S1: Vĩ độ
			Longitude, //S0: Kinh độ
			Speed,
			(Satelites < 4)? -1: ACC_State,
#ifdef USE_LigoSensor
			(fuel_sensor.sensor_found)? fuel_sensor.relative_level: -1,
#endif
#ifdef USE_DaviteqSensor
			(int) data_F,
#endif
//			gps.speed_km,
//			gps.speed_k,
			-1,
			-1,
			sim.Signal,
			sim.NetTech,
			date_time.Hour,
			date_time.Minute,
			date_time.Second,
			date_time.Date,
			date_time.Month,
			(date_time.Year + 2000),
			BoardParameter.driverID,
			sim.PhoneNumber,
			(sd.total_sectors_KiB - sd.free_sectors_KiB),
			sd.total_sectors_KiB);

	if((Longitude != 0) && (Latitude != 0)){
		if(http_post(&sim, API_POST_DATA_DEOCA, DataToPost) == SIM7600_OK){
			ServerConnect_DeoCa = 1;
			printf("Post data (%s) to Deo Ca's server completed\r\n", DataToPost);
		}
		else{
			ServerConnect_DeoCa = 0;
			printf("Post data to Deo Ca's server fail\r\n");
		}
	}
	ServerConnected = ServerConnect_IoTVision & ServerConnect_DeoCa;
#endif

#ifndef POST_SERVER_DEOCA
	ServerConnected = ServerConnect_IoTVision;
#endif
}

//char DataResponseToServer6[100];

#define GET_CODE_SERVER_IOTVISION
#define GET_CODE_SERVER_DEOCA

#ifdef GET_CODE_SERVER_IOTVISION
void GetCodeFromServerIoTVision(){
	char urlStr[100];
	memset(urlStr, 0, sizeof(urlStr));
	sprintf(urlStr, "%s%s", API_GET_CODE_IOTVISION, BoardParameter.ID);

	if(http_get(&sim, urlStr, _Server.HTTPGetResponse, &_Server.HTTPGetCode) == SIM7600_OK){
		if(_Server.HTTPGetCode == 200){
			if(_Server.HTTPGetResponse[1] == ']'){
				printf("Get code from IoTVision's server completed, no code found\r\n");
			}

			else if((_Server.HTTPGetResponse[1] != ']') && (_Server.HTTPGetResponse[1] != '}')){
				int foundRes = 0;
				int count = 0;
				for(int i = 0; i <= 256; ++i){
					if(_Server.HTTPGetResponse[i] == '"') ++count;
					if(count == 7){
						foundRes = i + 1;
						break;
					}
				}
				_Server.CodeGet = 0;
				for(int i = foundRes; i<=256; ++i){
					if(_Server.HTTPGetResponse[i] == ';') break;
					if(_Server.HTTPGetResponse[i] == '"') break;
					_Server.CodeGet *= 10;
					_Server.CodeGet += _Server.HTTPGetResponse[i] - 48;
				}

				printf("Get code from IoTVision's server completed, found code: %d\r\n", _Server.CodeGet);

				switch (_Server.CodeGet){
					case 1:
						//Send Firmware to server
						char FirmwareSendToServer[100];
						memset(FirmwareSendToServer, 0, sizeof(FirmwareSendToServer));
						sprintf(FirmwareSendToServer, "{\"ID\":\"%s\",\"PhienBan\":\"%s\"}", BoardParameter.ID, Version);

						char code1URL[100];
						memset(code1URL, 0, sizeof(code1URL));
						sprintf(code1URL, "%s%s", API_POST_FIRMWARE_VERSION_IOTVISION, BoardParameter.ID);

						if(http_post(&sim, code1URL, FirmwareSendToServer) == SIM7600_ERROR){
							printf("Cannot send firmware version to server\r\n");
						}
						else{
							printf("Send firmware version to server completed. Current firmware version: %s\r\n", Version);
						}
						break;

					case 2:
						//Update OTA...
						char DataResponseToServer2[100];
						memset(DataResponseToServer2, 0, sizeof(DataResponseToServer2));
						sprintf(DataResponseToServer2, "{\"ID\":\"%s\",\"S\":\"21\"}", BoardParameter.ID);

						if(http_post(&sim, API_POST_CODE_IOTVISION, DataResponseToServer2) == SIM7600_ERROR){
							printf("Send responce for OTA request to server error\r\n");
						}
						else{
//							UpdateFirmwareFromURL(OTA_FIRMWARE_URL);
							if(UpdateFirmwareFromURL(OTA_FIRMWARE_URL) == SIM7600_ERROR){
								printf("Download file for OTA from main URL error\r\n");
								if(UpdateFirmwareFromURL(OTA_FIRMWARE_URL_BACKUP) == SIM7600_ERROR){
									printf("Download file for OTA from backup URL error\r\n");
								}
								else{
									NVIC_SystemReset();
								}
							}
							else{
								NVIC_SystemReset();
							}
						}

	//
						break;

					case 3:
		//				[{"ID":"51A94887","S":"3;65M103466"}]
						char oldID[15];
						strcpy(oldID, BoardParameter.ID);

						memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));
						count = 0;
						for(int i = foundRes + 2; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '\"') break;
							BoardParameter.ID[count] = _Server.HTTPGetResponse[i];
							++count;
						}

						char DataResponseToServer[100];
						memset(DataResponseToServer, 0, sizeof(DataResponseToServer));
						sprintf(DataResponseToServer, "{\"ID\":\"%s\",\"S\":\"31\"}", oldID);

						if((http_post(&sim, API_POST_CODE_IOTVISION, DataResponseToServer) == SIM7600_ERROR) || (strlen(BoardParameter.ID) == 0)){

							memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));
							strcpy(BoardParameter.ID, oldID);
						}
						else{

							BoardParameter.update_status = 1;

							EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

							buzzer_used = 1;
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
							HAL_Delay(500);
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
							buzzer_used = 0;
						}

						break;
					case 4:
						int speed_limit_1_old = BoardParameter.speed_limit_1;
						BoardParameter.speed_limit_1 = 0;
						for(int i = foundRes + 2; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '|'){
								foundRes = i+1;
								break;
							}
							BoardParameter.speed_limit_1 *= 10;
							BoardParameter.speed_limit_1 += _Server.HTTPGetResponse[i] - 48;
						}
						int speed_limit_2_old = BoardParameter.speed_limit_2;
						BoardParameter.speed_limit_2 = 0;
						for(int i = foundRes; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '|'){
								foundRes = i+1;
								break;
							}
							BoardParameter.speed_limit_2 *= 10;
							BoardParameter.speed_limit_2 += _Server.HTTPGetResponse[i] - 48;
						}

						int speed_limit_3_old = BoardParameter.speed_limit_3;
						BoardParameter.speed_limit_3 = 0;
						for(int i = foundRes; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '\"'){
								break;
							}
							BoardParameter.speed_limit_3 *= 10;
							BoardParameter.speed_limit_3 += _Server.HTTPGetResponse[i] - 48;
						}

						char DataResponseToServer4[100];
						memset(DataResponseToServer4, 0, sizeof(DataResponseToServer4));
						sprintf(DataResponseToServer4, "{\"ID\":\"%s\",\"S\":\"41\"}", BoardParameter.ID);

						if(http_post(&sim, API_POST_CODE_IOTVISION, DataResponseToServer4) == SIM7600_ERROR){
							BoardParameter.speed_limit_1 = speed_limit_1_old;
							BoardParameter.speed_limit_2 = speed_limit_2_old;
							BoardParameter.speed_limit_3 = speed_limit_3_old;
						}
						else{
							EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

							buzzer_used = 1;
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
							HAL_Delay(500);
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
							buzzer_used = 0;
						}

						break;
					case 5:
	//					[{"ID":"51A94881","S":"5;27/05/2024"}]

						int date_old = BoardParameter.install_time.date;
						BoardParameter.install_time.date = 0;
						for(int i = foundRes + 2; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '/'){
								foundRes = i+1;
								break;
							}
							BoardParameter.install_time.date *= 10;
							BoardParameter.install_time.date += _Server.HTTPGetResponse[i] - 48;
						}

						int month_old = BoardParameter.install_time.month;
						BoardParameter.install_time.month = 0;
						for(int i = foundRes; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '/'){
								foundRes = i+1;
								break;
							}
							BoardParameter.install_time.month *= 10;
							BoardParameter.install_time.month += _Server.HTTPGetResponse[i] - 48;
						}

						int year_old = BoardParameter.install_time.year;
						BoardParameter.install_time.year = 0;
						for(int i = foundRes; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '\"'){
								break;
							}
							BoardParameter.install_time.year *= 10;
							BoardParameter.install_time.year += _Server.HTTPGetResponse[i] - 48;
						}

						char DataResponseToServer5[100];
						memset(DataResponseToServer5, 0, sizeof(DataResponseToServer5));
						sprintf(DataResponseToServer5, "{\"ID\":\"%s\",\"S\":\"51\"}", BoardParameter.ID);

						if(http_post(&sim, API_POST_CODE_IOTVISION, DataResponseToServer5) == SIM7600_ERROR){
							BoardParameter.install_time.date = date_old;
							BoardParameter.install_time.month = month_old;
							BoardParameter.install_time.year = year_old;
						}
						else{
							EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

							buzzer_used = 1;
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
							HAL_Delay(500);
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
							buzzer_used = 0;
						}

						break;

					case 6:
	//					[{"ID":"51A94881","S":"6;IoTVision"}]
						uint8_t old_check_in_out_state;
						old_check_in_out_state = BoardParameter.check_in_out_state;

						char old_driver_ID[20];
						strcpy(old_driver_ID, BoardParameter.driverID);
						memset(BoardParameter.driverID, 0, sizeof(BoardParameter.driverID));

						count = 0;
						for(int i = foundRes + 2; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '\"') break;
							BoardParameter.driverID[count] = _Server.HTTPGetResponse[i];
							++count;
						}
						DS3231_read_time(&date_time);

						char DataResponseToServer6[100];
	//					memset(DataResponseToServer6, 0, sizeof(DataResponseToServer6));

						int CodeForPost;
						if(BoardParameter.check_in_out_state == 0){
							BoardParameter.check_in_out_state = 1;

							if((ACC_State == 1) && (Speed > 0)) CodeForPost = 62;
							else CodeForPost = 61;

							sprintf(DataResponseToServer6, "{\"ID\":\"%s\",\"S\":\"%d;%s|%d:%d:%d %d/%d/%d|%d|%f,%f\"}", BoardParameter.ID, CodeForPost, BoardParameter.driverID,date_time.Hour, date_time.Minute, date_time.Second, date_time.Date, date_time.Month, (date_time.Year + 2000), BoardParameter.check_in_out_state, Longitude, Latitude);
						}
						else if(BoardParameter.check_in_out_state == 1){
							if(strcmp(old_driver_ID, BoardParameter.driverID) == 0){
								BoardParameter.check_in_out_state = 0;

								if((ACC_State == 1) && (Speed > 0)) CodeForPost = 62;
								else CodeForPost = 61;

								sprintf(DataResponseToServer6, "{\"ID\":\"%s\",\"S\":\"%d;|%d:%d:%d %d/%d/%d|%d|%f,%f\"}", BoardParameter.ID, CodeForPost, date_time.Hour, date_time.Minute, date_time.Second, date_time.Date, date_time.Month, (date_time.Year + 2000), BoardParameter.check_in_out_state, Longitude, Latitude);
								memset(BoardParameter.driverID, 0, sizeof(BoardParameter.driverID));
							}
							else{
								if((ACC_State == 1) && (Speed > 0)) CodeForPost = 62;
								else CodeForPost = 61;

								sprintf(DataResponseToServer6, "{\"ID\":\"%s\",\"S\":\"%d;%s|%d:%d:%d %d/%d/%d|%d|%f,%f\"}", BoardParameter.ID, CodeForPost, BoardParameter.driverID,date_time.Hour, date_time.Minute, date_time.Second, date_time.Date, date_time.Month, (date_time.Year + 2000), BoardParameter.check_in_out_state, Longitude, Latitude);
							}
						}

						if(http_post(&sim, API_POST_CODE_IOTVISION, DataResponseToServer6) == SIM7600_ERROR){
							BoardParameter.check_in_out_state = old_check_in_out_state;
							strcpy(BoardParameter.driverID, old_driver_ID);
						}
						else{
							EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));
							if(BoardParameter.check_in_out_state == 1){
								Bip(250);
								Bip(250);
							}
							else if(BoardParameter.check_in_out_state == 0){
	//							Bip(200);
								buzzer_used = 1;
								HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
								HAL_Delay(1000);
								HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
								buzzer_used = 0;
							}


						}
						PostDataToServer();

						break;
				}

				memset(_Server.HTTPGetResponse, 0, sizeof(_Server.HTTPGetResponse));
			}
		}
	}
	else{
		printf("Get code from IoTVision server error\r\n");
	}
}
#endif

#ifdef GET_CODE_SERVER_DEOCA
void GetCodeFromServerDeoCa(){
	char urlStrDCa[100];
	memset(urlStrDCa, 0, sizeof(urlStrDCa));
	sprintf(urlStrDCa, "%s%s", API_GET_CODE_DEOCA, BoardParameter.ID);

	if(http_get(&sim, urlStrDCa, _Server.HTTPGetResponse, &_Server.HTTPGetCode) == SIM7600_OK){
		if((_Server.HTTPGetCode == 200)){
			if(_Server.HTTPGetResponse[1] == '"'){
				int foundRes = 0;
				int count = 0;
				for(int i = 0; i <= 256; ++i){
					if(_Server.HTTPGetResponse[i] == '"') ++count;
					if(count == 7){
						foundRes = i + 1;
						break;
					}
				}
				_Server.CodeGet = 0;
				for(int i = foundRes; i<=256; ++i){
					if(_Server.HTTPGetResponse[i] == ';') break;
					if(_Server.HTTPGetResponse[i] == '"') break;
					_Server.CodeGet *= 10;
					_Server.CodeGet += _Server.HTTPGetResponse[i] - 48;
				}

				printf("Get code from Deo Ca's server completed, found code: %d\r\n", _Server.CodeGet);
				switch (_Server.CodeGet){
					case 1:
						//Send Firmware to server
						char FirmwareSendToServer[100];
						memset(FirmwareSendToServer, 0, sizeof(FirmwareSendToServer));
						sprintf(FirmwareSendToServer, "{\"ID\":\"%s\",\"S\":\"11;%s\"}", BoardParameter.ID, Version);

						if(http_post(&sim, API_POST_CODE_DEOCA, FirmwareSendToServer) == SIM7600_ERROR){
							printf("Cannot send firmware version to server\r\n");
						}
						else{
							printf("Send firmware version to server completed. Current firmware version: %s\r\n", Version);
						}
						break;

					case 2:
//						//Update OTA...
						char DataResponseToServer2[100];

						if(UpdateFirmwareFromURL(OTA_FIRMWARE_URL) == SIM7600_ERROR){
							printf("Download file for OTA from main URL error\r\n");
							if(UpdateFirmwareFromURL(OTA_FIRMWARE_URL_BACKUP) == SIM7600_ERROR){
								printf("Download file for OTA from backup URL error\r\n");

								memset(DataResponseToServer2, 0, sizeof(DataResponseToServer2));
								sprintf(DataResponseToServer2, "{\"ID\":\"%s\",\"S\":\"20\"}", BoardParameter.ID);

								if(http_post(&sim, API_POST_CODE_DEOCA, DataResponseToServer2) == SIM7600_ERROR){
									printf("Send responce for OTA request to server error\r\n");
								}
							}
							else{
								memset(DataResponseToServer2, 0, sizeof(DataResponseToServer2));
								sprintf(DataResponseToServer2, "{\"ID\":\"%s\",\"S\":\"21\"}", BoardParameter.ID);

								if(http_post(&sim, API_POST_CODE_DEOCA, DataResponseToServer2) == SIM7600_ERROR){
									printf("Send responce for OTA request to server error\r\n");
								}
								NVIC_SystemReset();
							}
						}
						else{
							memset(DataResponseToServer2, 0, sizeof(DataResponseToServer2));
							sprintf(DataResponseToServer2, "{\"ID\":\"%s\",\"S\":\"21\"}", BoardParameter.ID);

							if(http_post(&sim, API_POST_CODE_DEOCA, DataResponseToServer2) == SIM7600_ERROR){
								printf("Send responce for OTA request to server error\r\n");
							}
							NVIC_SystemReset();
						}
						break;

					case 3:
		//				[{"ID":"51A94887","S":"3;65M103466"}]
						char oldID[15];
						strcpy(oldID, BoardParameter.ID);

						memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));
						count = 0;
						for(int i = foundRes + 2; i <= 256; ++i){
							if(_Server.HTTPGetResponse[i] == '\"') break;
							BoardParameter.ID[count] = _Server.HTTPGetResponse[i];
							++count;
						}

						char DataResponseToServer[100];
						memset(DataResponseToServer, 0, sizeof(DataResponseToServer));
						sprintf(DataResponseToServer, "{\"ID\":\"%s\",\"S\":\"31\"}", oldID);

						if((http_post(&sim, API_POST_CODE_DEOCA, DataResponseToServer) == SIM7600_ERROR) || (strlen(BoardParameter.ID) == 0)){

							memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));
							strcpy(BoardParameter.ID, oldID);
						}
						else{
							BoardParameter.update_status = 1;

							EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

							buzzer_used = 1;
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
							HAL_Delay(500);
							HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
							buzzer_used = 0;
						}

						break;

				}
				memset(_Server.HTTPGetResponse, 0, sizeof(_Server.HTTPGetResponse));

//				printf("Get code from Deo Ca's server completed, found code:\r\n");


			}
			else if(_Server.HTTPGetResponse[1] == '}'){
				printf("Get code from Deo Ca's server completed, no code found\r\n");
			}
		}
		else{
			printf("Get code from Deo Ca's server error\r\n");
		}




	}
}
#endif

void GetCodeFromServer(){
#ifdef GET_CODE_SERVER_IOTVISION
		GetCodeFromServerIoTVision();
#endif

#ifdef GET_CODE_SERVER_DEOCA
		GetCodeFromServerDeoCa();
#endif
}

void UpdateRTC(){
	if(GetLBS(&sim) == SIM7600_OK){
		if(sim.LBS_Date.Year > 0){
			date_time.Day = sim.LBS_Date.Day;
			date_time.Date = sim.LBS_Date.Date;
			date_time.Month = sim.LBS_Date.Month;
			date_time.Year = sim.LBS_Date.Year;

			date_time.Hour = sim.LBS_Date.Hour;
			date_time.Minute = sim.LBS_Date.Minute;
			date_time.Second = sim.LBS_Date.Second;
			DS3231_write_time(&date_time);
		}
	}
	else if(http_get(&sim, API_GET_TIME_IOTVISION, _Server.HTTPGetResponse, &_Server.HTTPGetCode) == SIM7600_OK){
		if(_Server.HTTPGetCode == 200){
			int foundRes = 0;
			for(int i = 0; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}
			_Server.rtc.Day = (_Server.HTTPGetResponse[foundRes] - 48) + 1;
			foundRes += 1;
	//		Find Day
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}
			_Server.rtc.Date = 0;
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ','){
					foundRes = i + 1;
					break;
				}
				_Server.rtc.Date *= 10;
				_Server.rtc.Date += _Server.HTTPGetResponse[i] - 48;
			}
	//		Find Month
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}

			_Server.rtc.Month = 0;
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ','){
					foundRes = i + 1;
					break;
				}
				_Server.rtc.Month *= 10;
				_Server.rtc.Month += _Server.HTTPGetResponse[i] - 48;
			}

	//		Find Year
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}

			_Server.rtc.Year = 0;
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ','){
					foundRes = i + 1;
					_Server.rtc.Year -= 2000;
					break;
				}
				_Server.rtc.Year *= 10;
				_Server.rtc.Year += _Server.HTTPGetResponse[i] - 48;
			}

	//		Find Hour
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}

			_Server.rtc.Hour = 0;
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ','){
					foundRes = i + 1;
					break;
				}
				_Server.rtc.Hour *= 10;
				_Server.rtc.Hour += _Server.HTTPGetResponse[i] - 48;
			}

	//		Find Minute
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}

			_Server.rtc.Minute = 0;
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ','){
					foundRes = i + 1;
					break;
				}
				_Server.rtc.Minute *= 10;
				_Server.rtc.Minute += _Server.HTTPGetResponse[i] - 48;
			}

	//		Find Second
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == ':'){
					foundRes = i + 1;
					break;
				}
			}

			_Server.rtc.Second = 0;
			for(int i = foundRes; i <= 256; ++i){
				if(_Server.HTTPGetResponse[i] == '}') break;
				_Server.rtc.Second *= 10;
				_Server.rtc.Second += _Server.HTTPGetResponse[i] - 48;
			}

			if(_Server.rtc.Year > 0){
				date_time.Day = _Server.rtc.Day;
				date_time.Date = _Server.rtc.Date;
				date_time.Month = _Server.rtc.Month;
				date_time.Year = _Server.rtc.Year;

				date_time.Hour = _Server.rtc.Hour;
				date_time.Minute = _Server.rtc.Minute;
				date_time.Second = _Server.rtc.Second;
				DS3231_write_time(&date_time);
			}
		}
		memset(_Server.HTTPGetResponse, 0, sizeof(_Server.HTTPGetResponse));
	}
}

void RunMainProgram(){
// Timer 50ms
#ifdef FLAG_50ms
	if(_Flag.t50ms){

		_Flag.t50ms = 0;
	}
#endif

// Timer 100ms
#ifdef FLAG_100ms
	if(_Flag.t100ms){
		if((sd.card_detect == 1) && (sd.card_mouted == 0)){
			card_remount(&sd);
		}
		_Flag.t100ms = 0;
	}
#endif

// Timer 250ms
#ifdef FLAG_250ms
	if(_Flag.t250ms){

		_Flag.t250ms = 0;
	}
#endif

// Timer 500ms
#ifdef FLAG_500ms
	if(_Flag.t500ms){
//		Bip(250);

//		if(flag_Received == 1){
//			HAL_GPIO_WritePin(LED_SENSOR_GPIO_Port, LED_SENSOR_Pin, GPIO_PIN_SET);
//		}
//		else{
//			HAL_GPIO_WritePin(LED_SENSOR_GPIO_Port, LED_SENSOR_Pin, GPIO_PIN_RESET);
//		}
		_Flag.t500ms = 0;
	}
#endif

// Timer 1s
#ifdef FLAG_1s
	if(_Flag.t1s){

		_Flag.t1s = 0;
	}
#endif

// Timer 1500ms
#ifdef FLAG_1500ms
	if(_Flag.t1500ms){

		_Flag.t1500ms = 0;
	}
#endif

// Timer 2s
#ifdef FLAG_2s
	if(_Flag.t2s){

		GetCodeFromServer();

		_Flag.t2s = 0;
	}
#endif

// Timer 3s
#ifdef FLAG_3s
	if(_Flag.t3s){
		if(date_time.Year == 0){
			UpdateRTC();
		}

#ifdef RUN_SIM
		CheckSimConnect(&sim);
		GetSignal(&sim);
#endif

		Satelites = getSatelitesNum(&gps);
		if((Satelites < 4) && (sim.Signal > 0) && (sim.LBS_Loc.Latitude != 0) && (sim.LBS_Loc.Longitude != 0)){
			printf("Get location completed, using LBS location\r\n");
			GetLBS(&sim);
			Latitude = sim.LBS_Loc.Latitude;
			Longitude = sim.LBS_Loc.Longitude;
			Speed = 0;

			if((Latitude != 0) && (Longitude != 0) && ((BoardParameter.LastLatitude == 0) || (BoardParameter.LastLongitude == 0))){
				printf("Setting new backup location\r\n");
				BoardParameter.LastLatitude = Latitude;
				BoardParameter.LastLongitude = Longitude;
				EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));
			}
		}
		else if(Satelites >= 4){
			printf("Get location completed, using location from GPS module\r\n");
			Latitude = getLatitude(&gps);
			Longitude = getLongitude(&gps);
			Speed = getSpeed(&gps);
			if(Speed < 1) Speed = 0;

			if((Latitude != 0) && (Longitude != 0) && ((BoardParameter.LastLatitude == 0) || (BoardParameter.LastLongitude == 0))){
				printf("Setting new backup location\r\n");
				BoardParameter.LastLatitude = Latitude;
				BoardParameter.LastLongitude = Longitude;
				EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));
			}
		}

		if((Latitude == 0) && (Longitude == 0) && (BoardParameter.LastLatitude != 0) && (BoardParameter.LastLongitude != 0)){
			printf("Cannot get location, using old location found in EEPROM\r\n");
			Latitude = BoardParameter.LastLatitude;
			Longitude = BoardParameter.LastLongitude;
		}

		printf("https://www.google.com/maps/?q=%f,%f\r\n", Latitude, Longitude);
#ifdef USE_LigoSensor
		send_command_to_fuel_sensor(&fuel_sensor);
#endif
		_Flag.t3s = 0;
	}
#endif

// Timer 4s
#ifdef FLAG_4s
	if(_Flag.t4s){
//		GetCodeFromServer();
		_Flag.t4s = 0;
	}
#endif

// Timer 5s
#ifdef FLAG_5s
	if(_Flag.t5s){
//		PostDataToServer();
//		Bip();

		LogData();
		_Flag.t5s = 0;
	}
#endif

// Timer 6s
#ifdef FLAG_6s
	if(_Flag.t6s){

		_Flag.t6s = 0;
	}
#endif

// Timer 10s
#ifdef FLAG_10s
	if(_Flag.t10s){

		_Flag.t10s = 0;
	}
#endif

// Timer 15s
#ifdef FLAG_15s
	if(_Flag.t15s){
		if(sd.card_mouted){
			get_free_space(&sd);
			if(sd.free_sectors_KiB < 500000){
				char folder_to_delete[20];
				char path_to_delete[100];
				if(find_folder_name(&sd, BACKUP_PATH, 1, folder_to_delete) == FR_OK){
					sprintf(path_to_delete, "%s/%s", BACKUP_PATH, folder_to_delete);
					delete_folder(path_to_delete);
				}
			}
		}
#ifdef USE_ACC
		if(ACC_State == 1){
#endif
			PostDataToServer();
#ifdef USE_ACC
		}
#endif
		_Flag.t15s = 0;
	}
#endif

// Timer 30s
#ifdef FLAG_30s
	if(_Flag.t30s){
////		if(ACC_State == 1){
//			PostDataToServer();
////			Bip(50);
////		}
		_Flag.t30s = 0;
	}
#endif

// Timer 35s
#ifdef FLAG_35s
	if(_Flag.t35s){
		if((Latitude != 0) && (Longitude != 0) && (BoardParameter.LastLatitude != Latitude) && (BoardParameter.LastLongitude != Longitude)){
			printf("Backup current location\r\n");
			BoardParameter.LastLatitude = Latitude;
			BoardParameter.LastLongitude = Longitude;
			EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));
		}
		_Flag.t35s = 0;
	}
#endif

// Timer 15p
#ifdef FLAG_15p
	if(_Flag.t15p){


#ifdef USE_ACC
		if(ACC_State == 0){
#endif
			PostDataToServer();

#ifdef USE_ACC
		}
#endif
		_Flag.t15p = 0;
	}
#endif

// Timer 30p
#ifdef FLAG_30p
	if(_Flag.t30p){
		UpdateRTC();
		_Flag.t30p = 0;
	}
#endif

}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	char response[1024]; // Buffer for response
	int httpCode;  // For HTTP status code
	//const char* url = "https://nckh.assfa.net/postdata.php?ThietBi=station011&NhietDo=30&DoAm=50&AnhSang=20&Lati=10.849566&Longti=106.771740";
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_UART4_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  MX_TIM4_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
//  printf("DeoCa App");
  printf("\r\n");
  printf("DeoCa VEHICLE TRACKING\r\n");

  printf("--------------------------------------------------\r\n");
  printf("Firmware version: %s\r\n", Version);
  STM32_GetUniqueID(uniqueID_str);
  printf("STM32 chip ID: %s\r\n", uniqueID_str);
  printf("--------------------------------------------------\r\n");

  printf("Waiting for starting board's services\r\n");

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_Base_Start_IT(&htim4);

  HAL_GPIO_WritePin(LED_PWR_GPIO_Port, LED_PWR_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_SENSOR_GPIO_Port, LED_SENSOR_Pin, GPIO_PIN_SET);

  ACC_State = HAL_GPIO_ReadPin(ACC_GPIO_Port, ACC_Pin);

  FRESULT fres = card_init(&sd, &hspi1);
  if(fres == FR_OK){
	  printf("Init SD card completed, sd card size: %ld/%ld (Kb)\r\n", (sd.total_sectors_KiB - sd.free_sectors_KiB), sd.total_sectors_KiB);
  }
  else{
	  printf("Init SD card error: %d\r\n", fres);
  }

  SHT31_Config(SHT31_ADDRESS_A,&hi2c1);

  // Init DS3231 RTC module
  rtc_init(&hi2c1);
  DS3231_read_time(&date_time);

  // Init EEPROM and read data
  EEPROM_init(&eeprom, &hi2c1, EEPROM_I2C_ADDRESS, EEPROM_SIZE);

//  clear_EEPROM(&eeprom);
//  BoardParameter.update_status = ;
//  strcpy(BoardParameter.ID, "65M103466");
//  strcpy(BoardParameter.ID, "0919806788");


//  strcpy(BoardParameter.driverID, "");
//  BoardParameter.check_in_out_state = 255;
//  EEPROM_write_bytes(&eeprom, 0x00, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

  if(EEPROM_read_bytes(&eeprom, 0x00, (uint8_t *)&BoardParameter, sizeof(BoardParameter)) == EEPROM_OK){
	  printf("Read data from EEPROM completed\r\n");
  }
  else{
	  printf("Init EEPROM error\r\n");
  }

  BoardParameter.LastLatitude = 10.853851;
  BoardParameter.LastLongitude = 106.770741;

  // Check OTA Flag in EEPROM to find out OTA state
  if(BoardParameter.OTA_Flag == OTA_COMPLETED){
	  BoardParameter.OTA_Flag = NO_OTA;
	  EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

	  printf("OTA completed, current firmware version: %s\r\n", Version);
  }
  else if(BoardParameter.OTA_Flag == ERR_NO_FIRMWARE_FILE_FOUND){
	  BoardParameter.OTA_Flag = NO_OTA;
	  EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

	  printf("OTA fail\r\n");
  }


#ifdef USE_LigoSensor
//  Command_Format(data_txCommand, data_rxResponse);
  fuel_sensor_init(&fuel_sensor, &huart1, 0x02);
  send_command_to_fuel_sensor(&fuel_sensor);
#endif

#ifdef USE_DaviteqSensor
  uart_init();
  HAL_UART_Receive_IT(&huart1, data_rxResponse, 22);
#endif


// Init GPS module
  printf("Init GPS module\r\n");

  if(GPS_Init(&gps, &huart3) == GPS_OK){
	  printf("Init GPS module completed\r\n");
  }
  else{
	  printf("Init GPS module error\r\n");
  }

// Init SIMCOM module
  printf("Init SIMCOM module\r\n");

  if(SIMInit(&sim,
		  	 &huart2,
			 &hdma_usart2_rx,
			 SIM_PWR_Pin,
			 SIM_PWR_GPIO_Port,
			 SIM_RST_Pin,
			 SIM_RST_GPIO_Port,
			 SIM_HRST_Pin,
			 SIM_HRST_GPIO_Port,
			 SIM_STATUS_Pin,
			 SIM_STATUS_GPIO_Port) == SIM7600_OK){
	  printf("Init SIMCOM module completed\r\n");
  }
  else{
	  printf("Init SIMCOM module error\r\n");
  }
  HAL_Delay(4000);

  if(BoardParameter.update_status == 0){
	  printf("Found new EEPROM, setting up new device!!!\r\n");

	  clear_EEPROM(&eeprom);

	  char urlStr[100];
	  memset(urlStr, 0, sizeof(urlStr));
	  sprintf(urlStr, "%s%s", API_GENERATE_SERI_NUMBER, API_GENERATE_PREFIX);

	  if(http_get(&sim, urlStr, _Server.HTTPGetResponse, &_Server.HTTPGetCode) == SIM7600_OK){
		 if(_Server.HTTPGetCode == 200){
			 if(_Server.HTTPGetResponse[1] == '}'){
				 printf("Generate S/N number error\r\n");
			 }
			 else if(_Server.HTTPGetResponse[1] == '"'){
				 int count = 0;
				 memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));

				 for(int i = 10; i <= 28; ++i){
					 if(_Server.HTTPGetResponse[i] == '"') break;
					 BoardParameter.ID[count] = _Server.HTTPGetResponse[i];
					 ++count;
				 }

				 BoardParameter.update_status = 1;
				 BoardParameter.OTA_Flag = NO_OTA;
				 if(EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter)) == EEPROM_OK){
					 printf("Get new ID completed, current ID: %s\r\n", BoardParameter.ID);
//					 printf("Generate S/N number completed, new S/N number is %s, setting up new device\r\n");
				 }


			 }
		 }
	  }


//	  BoardParameter.update_status = 0;
//
//	  memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));
//	  strcpy(BoardParameter.ID, sim.PhoneNumber);
//
//	  BoardParameter.OTA_Flag = NO_OTA;
//
//	  EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));
  }

//  if(BoardParameter.update_status == 0){
//	  printf("Found new EEPROM, checking sim connect...\r\n");
//	  if(sim.SimConnect == 1){
//		  printf("Found SIM card with phone number: %s, setting up new device!!!\r\n", sim.PhoneNumber);
//
//		  clear_EEPROM(&eeprom);
//
//		  BoardParameter.update_status = 0;
//
//		  memset(BoardParameter.ID, 0, sizeof(BoardParameter.ID));
//		  strcpy(BoardParameter.ID, sim.PhoneNumber);
//
//		  BoardParameter.OTA_Flag = NO_OTA;
//
//		  EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));
//	  }
//	  else{
//		  printf("SIM Card not found, please insert sim card and press reset!!!\r\n");
//	  }
//  }

  printf("Completed all setup\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  printf("---------------------------------------------------------------------------------------\r\n");
	  RunMainProgram();


//	  // Đọc dữ liệu từ cảm biến
//	      if(SHT31_GetData(SHT31_Periodic, SHT31_Medium, SHT31_NON_Stretch, SHT31_1) == SHT31_OK) {
//	          // Lưu giá trị vào biến
//	          temperature = SHT31_GetTemperature();
//	          humidity = SHT31_GetHumidity();
//
//	          // In ra để debug
//	          printf("Temperature: %.2f°C, Humidity: %.2f%%\r\n", temperature, humidity);
//
//	          // Tạo URL với dữ liệu cảm biến
//	          char dynamicUrl[256];
//	          sprintf(dynamicUrl, "https://nckh.assfa.net/postdata.php?ThietBi=station011&NhietDo=%.2f&DoAm=%.2f&AnhSang=20&Lati=%.6f&Longti=%.6f",
//	                  temperature,
//	                  humidity,
//	                  BoardParameter.LastLatitude,
//	                  BoardParameter.LastLongitude);
//
//	          // Gửi request HTTP
//	          int result = http_get(&sim, dynamicUrl, response, &httpCode);
//
//	          if (result == SIM7600_OK && httpCode == 200) {
//	              printf("Data sent successfully\n");
//	          } else {
//	              printf("Failed to send data\n");
//	          }
//	      } else {
//	          printf("Failed to read SHT31 sensor\r\n");
//	      }
//
//	      // Delay giữa các lần đọc
//	      HAL_Delay(60000); // 1 phút
//	  printf("---------------------------------------------------------------------------------------\r\n");



//	  http_get(&sim, API_GET_TIME_IOTVISION, _Server.HTTPGetResponse, &_Server.HTTPGetCode);
//	  HAL_Delay(500);
//	  Bip();
//	  GetCodeFromServer();
//	  HAL_Delay(1000);

//	  luu("/Backup/text.txt", "")
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 128;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 63999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 49;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 63999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 249;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 63999;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 499;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 19200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SD_CS_Pin|SIM_PWR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SIM_RST_GPIO_Port, SIM_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SIM_HRST_GPIO_Port, SIM_HRST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LED_PWR_Pin|LED_SENSOR_Pin|LED_DRIVER_Pin|LED_SERVER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 PE4 PE5
                           PE6 PE7 PE9 PE10
                           PE11 PE12 PE13 PE14
                           PE15 PE0 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PC13 PC14 PC15 PC0
                           PC1 PC3 PC8 PC9
                           PC10 PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : ACC_Pin */
  GPIO_InitStruct.Pin = ACC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ACC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CD_Pin */
  GPIO_InitStruct.Pin = SD_CD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SD_CD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_RST_Pin */
  GPIO_InitStruct.Pin = SIM_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SIM_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_STATUS_Pin */
  GPIO_InitStruct.Pin = SIM_STATUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SIM_STATUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB2 PB12 PB13 PB14
                           PB15 PB3 PB4 PB5
                           PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_HRST_Pin */
  GPIO_InitStruct.Pin = SIM_HRST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SIM_HRST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD15
                           PD0 PD1 PD2 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_PWR_Pin */
  GPIO_InitStruct.Pin = SIM_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SIM_PWR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_PWR_Pin LED_SENSOR_Pin LED_DRIVER_Pin LED_SERVER_Pin */
  GPIO_InitStruct.Pin = LED_PWR_Pin|LED_SENSOR_Pin|LED_DRIVER_Pin|LED_SERVER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief Print the characters to UART (printf).
  * @retval int
  */
#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the UART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

  return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
