/*
 * sim7600.h
 *
 *  Created on: May 18, 2024
 *      Author: Selina
 */

#ifndef INC_SIM7600_H_
#define INC_SIM7600_H_

#include "main.h"
#include <string.h>
#include <limits.h>
#include <math.h>


#define 	SIM_Delay(x)				HAL_Delay(x)

#define 	SIM_RX_BUFF_SIZE			1100

#define 	MAX_HTTP_ERROR_COUNT		5

// Library's define
#define 	SIM_Debug

#define 	SIM7600_OK 					1
#define 	SIM7600_ERROR				0
#define 	SIM7600_SIGNAL_ERROR		2

#define		SIM_CARD_CONNECTED			1
#define		SIM_CARD_NOT_FOUND			0


//-----General command-----------
#define 	SIM_OK          			"OK\r\n"
#define 	SIM_ERROR					"ERROR\r\n"
#define 	SIM_AT						"AT\r\n"
#define 	SIM_ATE0					"ATE0\r\n"
#define 	SIM_Syntax					"\r\n"

typedef struct{
    float Latitude;
    float Longitude;
}sim7600_locate_t;

typedef struct{
	int Day;

	int Date;
	int Month;
	int Year;
	int Hour;
	int Minute;
	int Second;
}sim7600_date_t;

typedef struct{
	UART_HandleTypeDef *huart;
	DMA_HandleTypeDef *hdma;

	uint16_t PowerPin;
	GPIO_TypeDef *PowerPort;

	uint16_t ResetPin;
	GPIO_TypeDef *ResetPort;

	uint16_t HResetPin;
	GPIO_TypeDef *HResetPort;

	uint16_t StatusPin;
	GPIO_TypeDef *StatusPort;
}sim7600_hardware_t;

typedef struct {
	sim7600_hardware_t hardware;

	//Trạng thái kết nối Bật tắt module (Module đang tắt: 0, Module đang bật: 1)
	uint8_t ModulePower;
    //Trạng thái kết nối Module (Không tìm thấy module: 0, Đã tìm thấy module: 1)
    uint8_t ModuleConnect;
    //Trạng thái kết nối sim (Chưa gắn sim: 0, Đã gắn sim: 1)
	uint8_t SimConnect;
	//Trạng thái sóng Sim (0: mất sóng, 1-4)
	int Signal;
	// SIM's RSSI
	int RSSI;
	//Công nghệ mạng (Mất sóng: 0, 2G: 2, CDMA: 3, LTE: 4)
	uint8_t NetTech;
    //Trạng thái chuyển mạng (Bật: 1, Tắt: 0)
    uint8_t Roaming;
    // Trạng thái kết nối HTTP
    int HTTPInit;

    char PhoneNumber[15];

    sim7600_date_t GPS_Date;

    sim7600_locate_t GPS_Loc;

    sim7600_date_t LBS_Date;

    sim7600_locate_t LBS_Loc;

    int x;
    int SuperviseHTTPError;

    char buffer[SIM_RX_BUFF_SIZE];

#ifdef SIM_Debug
    int SIMResetCount;

    int DebugLoop;
	char response[50];
	char DebugStr[100];

	char DebugResponceStr[60];

	char DebugLBSResponceStr[60];

	int DebugHTTPGetCode;
	int DebugHTTPGetDataLen;
	int CountHTTPGetOK;
	int CountHTTPGetError;

	int DebugHTTPPostCode;
	int DebugHTTPPostDataLen;
	int CountHTTPPostOK;
	int CountHTTPPostError;
#endif

} sim7600_t;

void SIM_UART_DMA_CallBack(sim7600_t *_sim);
void CheckSIMStatus(sim7600_t *_sim);
void ClearBuffer(sim7600_t *_sim);

int WaitForResponse(sim7600_t *_sim, char *response, long long timeout, uint8_t clear_buffer);
int WaitForResponceTrueFalse(sim7600_t *_sim, char *responceTrue, char *responceFalse, long long timeout, uint8_t clear_buffer);

int SendCommand(sim7600_t *_sim, char *command, char *response, long long timeout, uint8_t clear_buffer);
int SendCommandCheckTrueFalse(sim7600_t *_sim, char *command, char *responceTrue, char *responceFalse, long long timeout, uint8_t clear_buffer);

//int SIMInit(sim7600_t *_sim, UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma, uint16_t PowerPin, GPIO_TypeDef *PowerPort, uint16_t ResetPin, GPIO_TypeDef *ResetPort, uint16_t StatusPin, GPIO_TypeDef *StatusPort);
int SIMInit(sim7600_t *_sim, UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma, uint16_t PowerPin, GPIO_TypeDef *PowerPort, uint16_t ResetPin, GPIO_TypeDef *ResetPort, uint16_t HResetPin, GPIO_TypeDef *HResetPort, uint16_t StatusPin, GPIO_TypeDef *StatusPort);

int PowerOn(sim7600_t *_sim);
int ResetModule(sim7600_t *_sim);

int StartModuleService(sim7600_t *_sim);

int ConnectToModule(sim7600_t *_sim);
int CheckSimConnect(sim7600_t *_sim);
int GetPhoneNumber(sim7600_t *_sim);

int GetLBS(sim7600_t *_sim);
int GetRSSI(sim7600_t *_sim);
int GetSignal(sim7600_t *_sim);

int DownloadFile2(sim7600_t *_sim, char *url, char *file_link, int *httpCode);
int DownloadFile(sim7600_t *_sim, char *url, char *file_link, char *response, int *httpCode);
//int DownloadFileFromServer(sim7600_t *_sim, char *url, char *folder_path, char *file_name, char *response, int *httpCode);

int http_get(sim7600_t *_sim, char *url, char *response, int *httpCode);
int http_post(sim7600_t *_sim, char *url, char *data);

#endif /* INC_SIM7600_H_ */
