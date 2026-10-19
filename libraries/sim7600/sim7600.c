/*
 * sim7600.c
 *
 *  Created on: May 18, 2024
 *      Author: Selina
 *      Version: 1.1
 */

#include "sim7600.h"
#include "../Inc/sd.h"

#define PRIME 101 // Số nguyên tố để tính mã băm

int findSubstring(char *str, char *substr) {
    int len_str = strlen(str);
    int len_substr = strlen(substr);
    int i, j;
    int hash_str = 0; // Mã băm của chuỗi str
    int hash_substr = 0; // Mã băm của chuỗi substr
    int h = 1;

    // Tính h = (256^(len_substr-1)) % PRIME
    for (i = 0; i < len_substr - 1; i++) {
        h = (h * 256) % PRIME;
    }

    // Tính mã băm cho chuỗi substr và len_substr ký tự đầu tiên của chuỗi str
    for (i = 0; i < len_substr; i++) {
        hash_substr = (256 * hash_substr + (unsigned char)substr[i]) % PRIME;
        hash_str = (256 * hash_str + (unsigned char)str[i]) % PRIME;
    }

    // Duyệt qua các ký tự của chuỗi str
    for (i = 0; i <= len_str - len_substr; i++) {
        // Nếu mã băm của chuỗi substr và len_substr ký tự tiếp theo của chuỗi str trùng nhau
        if (hash_substr == hash_str) {
            // Kiểm tra từng ký tự của chuỗi substr và chuỗi con tương ứng trong chuỗi str
            for (j = 0; j < len_substr; j++) {
                if (str[i + j] != substr[j])
                    break;
            }
            // Nếu tất cả các ký tự đều trùng khớp
            if (j == len_substr)
                return i;
        }
        // Tính lại mã băm cho len_substr ký tự tiếp theo của chuỗi str
        if (i < len_str - len_substr) {
            hash_str = (256 * (hash_str - (unsigned char)str[i] * h) + (unsigned char)str[i + len_substr]) % PRIME;
            if (hash_str < 0)
                hash_str = (hash_str + PRIME);
        }
    }
    return -1; // Trả về -1 nếu không tìm thấy chuỗi con
}

// Uart DMA CallBack
void SIM_UART_DMA_CallBack(sim7600_t *_sim){
	HAL_UARTEx_ReceiveToIdle_DMA(_sim->hardware.huart, (uint8_t *)_sim->buffer, SIM_RX_BUFF_SIZE);
}

void CheckSIMStatus(sim7600_t *_sim){
	GetLBS(_sim);
	GetSignal(_sim);
}

// Clear UART Buffer
void ClearBuffer(sim7600_t *_sim){
	HAL_UART_DMAStop(_sim->hardware.huart);
	HAL_UARTEx_ReceiveToIdle_DMA(_sim->hardware.huart, (uint8_t *)_sim->buffer, SIM_RX_BUFF_SIZE);
	memset(_sim->buffer, 0, SIM_RX_BUFF_SIZE);
}

// Wait until SIMCOM module response data
// If timeout: return -1
// If find value: return the first char's address
int WaitForResponse(sim7600_t *_sim, char *response, long long timeout, uint8_t clear_buffer){
    int found;
    long long time = timeout;
#ifdef SIM_Debug
//    memset(_sim->response, 0, sizeof(_sim->response));
    strcpy(_sim->DebugStr, response);
#endif
    while(time >= 0){
        found = findSubstring(_sim->buffer, response);
		_sim->x = found;

        if(found != -1){
            if(clear_buffer){
                ClearBuffer(_sim);
            }
#ifdef SIM_Debug
//	strcpy(_sim->response,"FoundSubStr");
#endif
            return found;
        }
        time = time - 5;
        SIM_Delay(5);
    }

#ifdef SIM_Debug
//	strcpy(_sim->response,"TIME OUT");
#endif

    if(clear_buffer){
        ClearBuffer(_sim);
    }
    return -1;
}

// Wait until SIMCOM module response data and check which data recieved
// If timeout, return -1
// If found response true, return 1
// If found response false, return 0
int WaitForResponceTrueFalse(sim7600_t *_sim, char *responceTrue, char *responceFalse, long long timeout, uint8_t clear_buffer){
    int foundTrue;
    int foundFalse;
    long long time = timeout;

    while(time >= 0){
        foundTrue = findSubstring(_sim->buffer, responceTrue);

        if(foundTrue != -1){
            if(clear_buffer){
                ClearBuffer(_sim);
            }
#ifdef SIM_Debug
            memset(_sim->response, 0, sizeof(_sim->response));
            strcpy(_sim->response,"FoundSubStrTrue");
#endif
            return 1;
        }

        foundFalse = findSubstring(_sim->buffer, responceFalse);

        if(foundFalse != -1){
            if(clear_buffer){
                ClearBuffer(_sim);
            }
#ifdef SIM_Debug
            memset(_sim->response, 0, sizeof(_sim->response));
            strcpy(_sim->response,"FoundSubStrFalse");
#endif
            return 0;
        }

        time = time - 10;
        SIM_Delay(10);
    }

    if(clear_buffer){
        ClearBuffer(_sim);
    }
    return -1;
}

int SendCommand(sim7600_t *_sim, char *command, char *response, long long timeout, uint8_t clear_buffer){
	uint16_t length = strlen(command);
	if(HAL_UART_Transmit(_sim->hardware.huart, (uint8_t*)command, length, 1000) != HAL_OK){
		return SIM7600_ERROR;
	}
	else{
		return WaitForResponse(_sim, response, timeout, clear_buffer);
	}
}

int SendCommandCheckTrueFalse(sim7600_t *_sim, char *command, char *responceTrue, char *responceFalse, long long timeout, uint8_t clear_buffer){
	uint16_t length = strlen(command);
	if(HAL_UART_Transmit(_sim->hardware.huart, (uint8_t*)command, length, 1000) != HAL_OK){
		return SIM7600_ERROR;
	}
	else{
		return WaitForResponceTrueFalse(_sim, responceTrue, responceFalse, timeout, clear_buffer);
	}
}

// Init module
int SIMInit(sim7600_t *_sim, UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma, uint16_t PowerPin, GPIO_TypeDef *PowerPort, uint16_t ResetPin, GPIO_TypeDef *ResetPort, uint16_t HResetPin, GPIO_TypeDef *HResetPort, uint16_t StatusPin, GPIO_TypeDef *StatusPort){
	_sim->hardware.huart = huart;
	_sim->hardware.hdma = hdma;
// Power pin
	_sim->hardware.PowerPin = PowerPin;
	_sim->hardware.PowerPort = PowerPort;
// Reset Pin
	_sim->hardware.ResetPin = ResetPin;
	_sim->hardware.ResetPort = ResetPort;

// Status Pin
	_sim->hardware.StatusPin = StatusPin;
	_sim->hardware.StatusPort = StatusPort;

	_sim->hardware.HResetPin = HResetPin;
	_sim->hardware.HResetPort = HResetPort;

	HAL_GPIO_WritePin(_sim->hardware.HResetPort, _sim->hardware.HResetPin, GPIO_PIN_SET);

	HAL_UARTEx_ReceiveToIdle_DMA(_sim->hardware.huart, (uint8_t *)_sim->buffer, SIM_RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(_sim->hardware.hdma, DMA_IT_HT);

	return StartModuleService(_sim);
}

/*
 * When call this function, It'll power on module
 * */
int PowerOn(sim7600_t *_sim){
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"POWER ON");
#endif
	HAL_GPIO_WritePin(_sim->hardware.PowerPort, _sim->hardware.PowerPin, GPIO_PIN_SET);
	SIM_Delay(600);
	HAL_GPIO_WritePin(_sim->hardware.PowerPort, _sim->hardware.PowerPin, GPIO_PIN_RESET);
	SIM_Delay(1000);
	return SIM7600_OK;
}

/*
 * When call this function, It will pull high module's reset pin and pull
 * low again to reset module.
 * */
int ResetModule(sim7600_t *_sim){
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"RESET MODULE");
#endif
	HAL_GPIO_WritePin(_sim->hardware.ResetPort, _sim->hardware.ResetPin, GPIO_PIN_SET);
	SIM_Delay(300);
	HAL_GPIO_WritePin(_sim->hardware.ResetPort, _sim->hardware.ResetPin, GPIO_PIN_RESET);

	while(1){

#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"WAIT FOR MODULE READY");
#endif
		if(WaitForResponse(_sim, "PB DONE", 5000, 0) != -1){
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"Found module with sim inserted");
#endif
			printf("Found SIMCOM module with sim inserted\r\n");
			break;
		}
		else if(WaitForResponse(_sim, "SIM not inserted", 5000, 0) != -1){
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"Found module without sim card");
#endif
			printf("Found SIMCOM module without sim inserted, please insert sim card!!!\r\n");
			break;
		}
		SIM_Delay(1000);
	}

#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"RESET MODULE OK");
	++_sim->SIMResetCount;
#endif
	SIM_Delay(2000);
	return SIM7600_OK;
}

/*
 *
 * */
int StartModuleService(sim7600_t *_sim){

	if(PowerOn(_sim) == SIM7600_OK){
		if(ResetModule(_sim) == SIM7600_OK){
			_sim->ModulePower = 1;
		}
		else{
			_sim->ModulePower = 0;
			return SIM7600_ERROR;
		}
	}

	if(ConnectToModule(_sim) == SIM7600_OK){
		_sim->ModuleConnect = 1;
	}
	else{
		_sim->ModuleConnect = 0;
		return SIM7600_ERROR;
	}

	CheckSimConnect(_sim);
	SIM_Delay(4000);

	return SIM7600_OK;
}

/*
 * When call this function, It'll send "AT" command to module
 * If connect to module complete, It'll responce "OK"
 * */
int ConnectToModule(sim7600_t *_sim){
//	Send AT
	int Check = 0;
	while(SendCommand(_sim, SIM_AT, SIM_OK, 1000, 1) == -1){
		++Check;
		if(Check > 30){
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"Send AT error");
#endif
			_sim->ModuleConnect = 0;
			return SIM7600_ERROR;
		}
	}
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"Send AT OK");
#endif

// Send ATE0
	if(SendCommand(_sim, SIM_ATE0, SIM_OK, 1000, 1) != -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Send ATE0 OK");
#endif
	}
	else{
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Send ATE0 error");
#endif
		_sim->ModuleConnect = 0;
		return SIM7600_ERROR;
	}
	_sim->ModuleConnect = 1;
	return SIM7600_OK;
}

/*
 * This function'll check the sim card connection.
 * */
int CheckSimConnect(sim7600_t *_sim){
//	//	Setup Hotswap Level
//	if(SendCommand(_sim, "AT+UIMHOTSWAPLEVEL=1\r\n", SIM_OK, 1000, 1) == -1){
//#ifdef SIM_Debug
//		strcpy(_sim->response,"SETUP HOTSWAP LEVEL ERROR");
//#endif
//		return SIM7600_ERROR;
//	}
//	Check SIM connect
	if(SendCommand(_sim, "AT+CPIN?\r\n", "+CPIN: READY", 1000, 1) == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"NO SIM");
#endif
		_sim->SimConnect = 0;
		return SIM7600_ERROR;
	}

#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"SIM CONNECTED");
#endif

	GetPhoneNumber(_sim);

	_sim->SimConnect = 1;
	return SIM7600_OK;
}

// Get SIMCardPhoneNumber
int GetPhoneNumber(sim7600_t *_sim){
	int foundRes = SendCommand(_sim, "AT+CNUM\r\n", "+CNUM:", 2000, 0);
	if(foundRes == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Get Phone Number ERROR");
#endif
		ClearBuffer(_sim);
		return SIM7600_ERROR;
	}

	char DataResponseStr[60];
	int count = 0;
	for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
		if(_sim->buffer[i] == '\r') break;
		DataResponseStr[count] = _sim->buffer[i];
		++count;
	}
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->DebugResponceStr, DataResponseStr);
#endif

	int AnalysisPointer = 0;
	count = 0;
	for(int i = AnalysisPointer; i <= 60; ++i){
		if(DataResponseStr[i] == '\"') ++count;
		if(count == 3){
			if(DataResponseStr[i+1] == '0') AnalysisPointer = i + 2;
			else AnalysisPointer = i + 4;
			break;
		}
	}
	count = 1;
	memset(_sim->PhoneNumber,0,strlen(_sim->PhoneNumber));
	_sim->PhoneNumber[0] = '0';

	for(int i = AnalysisPointer; i <= 60; ++i){
		if(DataResponseStr[i] == '\"') break;
		_sim->PhoneNumber[count] = DataResponseStr[i];
		++count;
	}

	ClearBuffer(_sim);

	return SIM7600_OK;
//	int DKD;
//	sscanf(DataResponseStr, "+CNUM: \"My Number\",\"%s\",%d", &_sim->PhoneNumber, DKD);
}

// Find how many date in a month
int dateOfMonth(int month, int year){
	if((month == 1) || (month == 3) || (month == 5 ) || (month == 7) || (month == 8) || (month == 10) || (month == 12)){
		return 31;
	}
	else if((month == 4) || (month == 6) || (month == 9) || (month == 11)){
		return 30;
	}
	else if (((month == 2) && ((year % 4 == 0) && (year % 100!= 0))) || (year%400 == 0)){
		return 29;
	}
	return 28;
}

// Find which day is from date, mon and year
int dayOfWeek(int date, int month, int year) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) {
        year -= 1;
    }
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + date) % 7;
}

// Find timezone from longitude
int get_timezone_offset(float longitude) {
    return (int)round(longitude / 15.0);
}

// Get LBS
int GetLBS(sim7600_t *_sim){
	GetSignal(_sim);
	if(_sim->Signal == 0){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"No Signal");
#endif
		return SIM7600_ERROR;
	}

	int foundRes = SendCommand(_sim, "AT+CLBS=4\r\n", "+CLBS:", 1000, 0);
	if(foundRes == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Get LBS Location Error");
#endif
		ClearBuffer(_sim);
		return SIM7600_ERROR;
	}
	SIM_Delay(50);

	char DataResponseStr[60];
	int count = 0;
	for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
		if(_sim->buffer[i] == '\r') break;
		DataResponseStr[count] = _sim->buffer[i];
		++count;
	}
#ifdef SIM_Debug
//	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->DebugLBSResponceStr, DataResponseStr);
#endif
	int DKD;
	sscanf(DataResponseStr, "+CLBS: 0,%f,%f,%d,%d/%d/%d,%d:%d:%d", &_sim->LBS_Loc.Latitude, &_sim->LBS_Loc.Longitude, &DKD, &_sim->LBS_Date.Year, &_sim->LBS_Date.Month, &_sim->LBS_Date.Date, &_sim->LBS_Date.Hour, &_sim->LBS_Date.Minute, &_sim->LBS_Date.Second);
	_sim->LBS_Date.Day = dayOfWeek(_sim->LBS_Date.Date, _sim->LBS_Date.Month, _sim->LBS_Date.Year) + 1;
	_sim->LBS_Date.Year -= 2000;

	_sim->LBS_Date.Hour += get_timezone_offset(_sim->LBS_Loc.Longitude);
	if(_sim->LBS_Date.Hour >= 24){
		_sim->LBS_Date.Hour -= 24;
		_sim->LBS_Date.Date += 1;

		if(_sim->LBS_Date.Date > dateOfMonth(_sim->LBS_Date.Month, _sim->LBS_Date.Year)){
			_sim->LBS_Date.Date = 1;
			++_sim->LBS_Date.Month;
			if(_sim->LBS_Date.Month > 12){
				_sim->LBS_Date.Month = 1;
				++_sim->LBS_Date.Year;
			}
		}
	}

	ClearBuffer(_sim);
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"Get LBS Location OK");
#endif

	return SIM7600_OK;
}

/*
 * Get Signal RSSI Function
 * */
int GetRSSI(sim7600_t *_sim){
	int foundRes = SendCommand(_sim, "AT+CSQ\r\n", "+CSQ:", 2000, 0);
	if(foundRes == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Get RSSI Error");
#endif
		ClearBuffer(_sim);
		return SIM7600_ERROR;
	}

	char DataResponseStr[50];
	int count = 0;
	for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
		if(_sim->buffer[i] == '\r') break;
		DataResponseStr[count] = _sim->buffer[i];
		++count;
	}

	int Ber;
	sscanf(DataResponseStr, "+CSQ: %d,%d", &_sim->RSSI, &Ber);
	ClearBuffer(_sim);
#ifdef SIM_Debug
//		strcpy(_sim->response,"Get RSSI OK");
#endif

	return SIM7600_OK;
}

/*
 * Get SIMCOM Module's RSSI and analyze to 4 level signal
 * */
int GetSignal(sim7600_t *_sim){
	if(GetRSSI(_sim) == SIM7600_OK){
		if((_sim->RSSI == 99) || (_sim->RSSI == 199) || (_sim->RSSI == 0) || (_sim->RSSI == 1) || (_sim->RSSI == 100) || (_sim->RSSI == 101)){
			_sim->Signal = 0;
			_sim->NetTech = 0;
		}
		else if((_sim->RSSI >= 2) && (_sim->RSSI <= 31)){
			_sim->NetTech = 4;
			if((_sim->RSSI >= 2) && (_sim->RSSI <= 7)) _sim->Signal = 1;
			else if((_sim->RSSI >= 8) && (_sim->RSSI <= 15)) _sim->Signal = 2;
			else if((_sim->RSSI >= 16) && (_sim->RSSI <= 23)) _sim->Signal = 3;
			else if((_sim->RSSI >= 24) && (_sim->RSSI <= 31)) _sim->Signal = 4;
		}
		else if((_sim->RSSI >= 102) && (_sim->RSSI <= 191)){
			_sim->NetTech = 3;
			if((_sim->RSSI >= 102) && (_sim->RSSI <= 122)) _sim->Signal = 1;
			else if((_sim->RSSI >= 123) && (_sim->RSSI <= 145)) _sim->Signal = 2;
			else if((_sim->RSSI >= 146) && (_sim->RSSI <= 168)) _sim->Signal = 3;
			else if((_sim->RSSI >= 169) && (_sim->RSSI <= 191)) _sim->Signal = 4;
		}
		return SIM7600_OK;
	}
	return SIM7600_ERROR;
}

/*
 * Setup HTTP connect's function for SIMCOM Module
 * */

int InitHTTP(sim7600_t *_sim){
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"Init HTTP");
#endif
	if(SendCommand(_sim, "AT+HTTPINIT\r\n", SIM_OK, 2000, 1) == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Init HTTP error");
#endif
		return SIM7600_ERROR;
	}
	return SIM7600_OK;
}

/*
 * Deinit HTTP connect's function for SIMCOM Module
 * */

int DeInitHTTP(sim7600_t *_sim){
	if(SendCommand(_sim, "AT+HTTPTERM\r\n", SIM_OK, 1000, 1) == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Stop HTTP error");
#endif
		return SIM7600_ERROR;
	}

#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Deinit HTTP completed");
#endif
	return SIM7600_OK;
}

/*
 * Setup HTTP's URL
 * */

int SetupHTTPUrl(sim7600_t *_sim, char *url){
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"Setup URL");
#endif
	char urlStr[200];
	memset(urlStr, 0, sizeof(urlStr));
	sprintf(urlStr,"AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
	if(SendCommand(_sim, urlStr, SIM_OK, 1000, 1) == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Setup HTTP URL error");
#endif
		return SIM7600_ERROR;
	}
	return SIM7600_OK;
}

/*
 * Supervise HTTP error.
 * If have too much error, this function will automatically reset module
 * */
int SuperviseHTTP(sim7600_t *_sim, int ErrCheck){
	if(ErrCheck == SIM7600_ERROR){
		if(((_sim->SimConnect == SIM_CARD_CONNECTED) && (_sim->Signal > 1)) || (SendCommand(_sim, SIM_AT, SIM_OK, 1000, 1) == -1)){
			++_sim->SuperviseHTTPError;
			if(_sim->SuperviseHTTPError > MAX_HTTP_ERROR_COUNT){
				printf("Max HTTP ERROR found, reset SIMCOM Module\r\n");
				StartModuleService(_sim);
				_sim->SuperviseHTTPError = 0;
			}
#ifdef SIM_Debug
//		strcpy(_sim->response,"HTTP GET ERROR");
			++_sim->CountHTTPGetError;
#endif
		}

		return SIM7600_ERROR;
	}

	_sim->SuperviseHTTPError = 0;
	return SIM7600_OK;
}

/*
 * HTTP GET FILE DOWNLOAD FUNCTION
 * Can't down load file if Signal < 3
 * */
int DownloadFile2(sim7600_t *_sim, char *url, char *file_link, int *httpCode){
	int ErrCheck = SIM7600_OK;
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"DOWNLOAD FILE...");
#endif
	SIM_Delay(1000);
	GetSignal(_sim);
	if(_sim->Signal < 3){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"No Signal for GET");
#endif
		return SIM7600_SIGNAL_ERROR;
	}
// Init HTTP
	if(InitHTTP(_sim) == SIM7600_ERROR){
		ErrCheck = SIM7600_ERROR;
		goto StopHttpGetDownloadFIle2;
	}
// Set URL for HTTP
	if(SetupHTTPUrl(_sim, url) == SIM7600_ERROR){
		ErrCheck = SIM7600_ERROR;
		goto StopHttpGetDownloadFIle2;
	}

//	Send HTTP GET request
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"Send Action GET");
#endif
	int foundRes = SendCommand(_sim, "AT+HTTPACTION=0\r\n", "+HTTPACTION: 0", 5000, 0);
	if(foundRes == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Send HTTP request error");
#endif
		ErrCheck = SIM7600_ERROR;
		goto StopHttpGetDownloadFIle2;
	}
	SIM_Delay(10);

	char DataResponseStr[50];
	memset(DataResponseStr, 0, sizeof(DataResponseStr));

	int count = 0;
	for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
		if((_sim->buffer[i] == '\r') && (_sim->buffer[i+1] == '\n')) break;
		DataResponseStr[count] = _sim->buffer[i];
		++count;
	}
	int DataResponseLen = -1;

	int analysisHttpCode;

	sscanf(DataResponseStr, "+HTTPACTION: 0,%d,%d", &analysisHttpCode, &DataResponseLen);
	*httpCode = analysisHttpCode;

	if(*httpCode == 200){
#ifdef SIM_Debug
		strcpy(_sim->DebugResponceStr, DataResponseStr);
		_sim->DebugHTTPGetCode = *httpCode;
		_sim->DebugHTTPGetDataLen = DataResponseLen;
#endif
//		ClearBuffer(_sim);
		FIL         fil;                  //File handle
		FRESULT     fres;                 //Result after operations

		fres = f_open(&fil, file_link, FA_WRITE | FA_OPEN_ALWAYS);
		if(fres == FR_OK){
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"OPEN FILE OK");
			printf("Downloading\r\n");
//			HAL_UART_Transmit(&GLOBAL_DEBUG_UART_PORT, (uint8_t *)"Downloading", 11, 100);
#endif
			int TimesToGet, DataLengthLastGet;
			TimesToGet = DataResponseLen / 500;
			DataLengthLastGet = DataResponseLen - (TimesToGet * 500);
			_sim->DebugLoop = TimesToGet;

			int countCheck;

			for(int j = 0; j < TimesToGet; ++j){
				int recent_read_length = 0;
				char command[50];
				while(recent_read_length < 500){
					int length_to_read = 500 - recent_read_length;
					ClearBuffer(_sim);
					memset(command, 0, sizeof(command));
					sprintf(command, "AT+HTTPREAD=0,%d\r\n", length_to_read);
					foundRes = SendCommand(_sim, command, "+HTTPREAD:", 10000, 0);
					if(foundRes == -1){
#ifdef SIM_Debug
						memset(_sim->response, 0, sizeof(_sim->response));
						strcpy(_sim->response,"Read HTTP data ERROR FILE");
#endif
						ErrCheck = SIM7600_ERROR;
						goto CloseFile2;
					}
					SIM_Delay(50);
					int length_read = 0;

					for(int i = foundRes; i <= foundRes + 22; ++i){
						if(_sim->buffer[i] == ','){
							foundRes = i + 1;
							break;
						}
					}

					for(int i = foundRes; i <= foundRes + 5; ++i){
						if((_sim->buffer[i] == '\r') && (_sim->buffer[i+1] == '\n')){
							foundRes = i + 2;
							goto WriteFile2;
						}
						length_read *= 10;
						length_read += _sim->buffer[i] - 48;
					}
					goto CloseFile2;

					WriteFile2:
					recent_read_length += length_read;

					countCheck = 0;
					while(1){
						if((_sim->buffer[foundRes + length_read + 2] == '+') && (_sim->buffer[foundRes + length_read + 3] == 'H') && (_sim->buffer[foundRes + length_read + 4] == 'T') && (_sim->buffer[foundRes + length_read + 5] == 'T') && (_sim->buffer[foundRes + length_read + 6] == 'P')) break;
#ifdef SIM_Debug
						memset(_sim->response, 0, sizeof(_sim->response));
						strcpy(_sim->response,"Wait HTTPREAD Again");
#endif
						SIM_Delay(100);
						++countCheck;
						if(countCheck > 50) {
#ifdef SIM_Debug
					memset(_sim->response, 0, sizeof(_sim->response));
					strcpy(_sim->response,"Wait HTTPREAD ERROR");
#endif
							ErrCheck = SIM7600_ERROR;
							goto CloseFile2;
						}
					}


					UINT bw;
					f_write(&fil, &_sim->buffer[foundRes], length_read, &bw);
				}

				--_sim->DebugLoop;
				printf(".");
			}

			//	Get another last data
			ClearBuffer(_sim);
			int recent_read_length = 0;
			char command[50];
			while(recent_read_length < DataLengthLastGet){
				int length_to_read = DataLengthLastGet - recent_read_length;
				ClearBuffer(_sim);
				memset(command, 0, sizeof(command));
				sprintf(command, "AT+HTTPREAD=0,%d\r\n", length_to_read);
				foundRes = SendCommand(_sim, command, "+HTTPREAD:", 10000, 0);
				if(foundRes == -1){
#ifdef SIM_Debug
					memset(_sim->response, 0, sizeof(_sim->response));
					strcpy(_sim->response,"Read HTTP data ERROR FILE");
#endif
					ErrCheck = SIM7600_ERROR;
					goto CloseFile2;
				}
//					HAL_Delay(50);
				int length_read = 0;

				for(int i = foundRes; i <= foundRes + 22; ++i){
					if(_sim->buffer[i] == ','){
						foundRes = i + 1;
						break;
					}
				}

				for(int i = foundRes; i <= foundRes + 5; ++i){
					if((_sim->buffer[i] == '\r') && (_sim->buffer[i+1] == '\n')){
						foundRes = i + 2;
						goto WriteFileLastGet2;
					}
					length_read *= 10;
					length_read += _sim->buffer[i] - 48;
				}
				goto CloseFile2;

				WriteFileLastGet2:
				recent_read_length += length_read;

				countCheck = 0;
				while(1){
					if((_sim->buffer[foundRes + length_read + 2] == '+') && (_sim->buffer[foundRes + length_read + 3] == 'H') && (_sim->buffer[foundRes + length_read + 4] == 'T') && (_sim->buffer[foundRes + length_read + 5] == 'T') && (_sim->buffer[foundRes + length_read + 6] == 'P')) break;
#ifdef SIM_Debug
					memset(_sim->response, 0, sizeof(_sim->response));
					strcpy(_sim->response,"Wait HTTPREAD Again");
#endif
					SIM_Delay(100);
					++countCheck;
					if(countCheck > 50) {
#ifdef SIM_Debug
				memset(_sim->response, 0, sizeof(_sim->response));
				strcpy(_sim->response,"Wait HTTPREAD ERROR");
#endif
						ErrCheck = SIM7600_ERROR;
						goto CloseFile2;
					}
				}


				UINT bw;
				f_write(&fil, &_sim->buffer[foundRes], length_read, &bw);
			}

#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response, "Get data completed");
			printf("\r\n");
			printf("Download file completed\r\n");

#endif
		    //close your file
			CloseFile2:
		    f_close(&fil);
		    ClearBuffer(_sim);

		    if(ErrCheck == SIM7600_ERROR){
		    	f_unlink(file_link);
		    	printf("Download file fail\r\n");
		    }
		}
	}
	else{
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"HTTP CODE RETURN ERROR");
#endif
	}

	//	Stop HTTP request
//	StopHttpGet:
	StopHttpGetDownloadFIle2:
//	ErrCheck = DeInitHTTP(_sim);
	DeInitHTTP(_sim);

	if(ErrCheck == SIM7600_OK){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Download file completed");
		++_sim->CountHTTPGetOK;
#endif
	}
	return SuperviseHTTP(_sim, ErrCheck);

////	Stop HTTP request
//	StopHttpGetDownloadFIle2:
//	ErrCheck = DeInitHTTP(_sim);
//
//#ifdef SIM_Debug
//	memset(_sim->response, 0, sizeof(_sim->response));
//	strcpy(_sim->response,"HTTP GET OK");
//	++_sim->CountHTTPGetOK;
//#endif
//	return SuperviseHTTP(_sim, ErrCheck);
}

/*
 * HTTP POST FUNCTION
 * */
int http_post(sim7600_t *_sim, char *url, char *data){
	int ErrCheck = SIM7600_OK;
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"HTTP POST");
#endif

	GetSignal(_sim);
	if(_sim->Signal == 0){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"No Signal for POST");
#endif
		return SIM7600_SIGNAL_ERROR;
	}

// Init HTTP
	if(InitHTTP(_sim) == SIM7600_ERROR){
		ErrCheck = SIM7600_ERROR;
		goto StopHttpPost;
	}
// Set URL for HTTP
	if(SetupHTTPUrl(_sim, url) == SIM7600_ERROR){
		ErrCheck = SIM7600_ERROR;
		goto StopHttpPost;
	}

	if(SendCommand(_sim, "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n", SIM_OK, 2000, 1) == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Set HTTPPARA ERROR");
#endif
		ErrCheck = SIM7600_ERROR;
		goto StopHttpPost;
	}

// Send HTTP DATA
	char httpDataStr[200];
	memset(httpDataStr, 0, sizeof(httpDataStr));

	int data_len = strlen(data);

	sprintf(httpDataStr,"AT+HTTPDATA=%d,1000\r\n", data_len);
#ifdef SIM_Debug
	strcpy(_sim->DebugStr, httpDataStr);
#endif
	SendCommand(_sim, httpDataStr, "DOWNLOAD", 2000, 1);
	SendCommand(_sim, data, SIM_OK, 2000, 1);

//	Send HTTP POST request
	int responce = SendCommandCheckTrueFalse(_sim, "AT+HTTPACTION=1\r\n", "+HTTPACTION: 1", SIM_ERROR, 30000, 0);

	if((responce == -1) || (responce == 0)){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Send HTTP request error");
#endif
		ErrCheck = SIM7600_ERROR;
	}
	else if(responce == 1){
		int foundRes = WaitForResponse(_sim, "+HTTPACTION: 1", 100, 0);
		if(foundRes == -1){
	#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"Send HTTP request error");
	#endif
			ErrCheck = SIM7600_ERROR;
			goto StopHttpPost;
		}
		SIM_Delay(5);

		char DataResponseStr[50];
		memset(DataResponseStr, 0, sizeof(DataResponseStr));
		int count = 0;
		for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
			if(_sim->buffer[i] == '\r') break;
			DataResponseStr[count] = _sim->buffer[i];
			++count;
		}

		int DataResponseLen = -1;

		int analysisHttpCode = -1;

		sscanf(DataResponseStr, "+HTTPACTION: 1,%d,%d", &analysisHttpCode, &DataResponseLen);

#ifdef SIM_Debug
		strcpy(_sim->DebugResponceStr, DataResponseStr);
		_sim->DebugHTTPPostCode = analysisHttpCode;
		_sim->DebugHTTPGetDataLen = DataResponseLen;
#endif

		if(analysisHttpCode != 200){
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"HTTP POST error");
	#endif
			ErrCheck = SIM7600_ERROR;
			goto StopHttpPost;
		}

	}

	ClearBuffer(_sim);

//	Stop HTTP request
	StopHttpPost:
	DeInitHTTP(_sim);
	if(ErrCheck == SIM7600_OK){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"HTTP POST OK");
		++_sim->CountHTTPGetOK;
#endif
	}
	return SuperviseHTTP(_sim, ErrCheck);
}

/*
 * HTTP GET FUNCTION
 * */
int http_get(sim7600_t *_sim, char *url, char *response, int *httpCode){
	int ErrCheck = SIM7600_OK;
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"HTTP GET");
#endif

	GetSignal(_sim);
	if(_sim->Signal == 0){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"No Signal for GET");
#endif
		return SIM7600_SIGNAL_ERROR;
	}

// Init HTTP
	if(InitHTTP(_sim) == SIM7600_ERROR){
		ErrCheck = SIM7600_ERROR;
		goto StopHttpGet;
	}
// Set URL for HTTP
	if(SetupHTTPUrl(_sim, url) == SIM7600_ERROR){
		ErrCheck = SIM7600_ERROR;
		goto StopHttpGet;
	}

//	Send HTTP GET request
#ifdef SIM_Debug
	memset(_sim->response, 0, sizeof(_sim->response));
	strcpy(_sim->response,"Send Action GET");
#endif
	int foundRes = SendCommand(_sim, "AT+HTTPACTION=0\r\n", "+HTTPACTION: 0", 30000, 0);
	if(foundRes == -1){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"Send HTTP request error");
#endif
		ErrCheck = SIM7600_ERROR;
		goto StopHttpGet;
	}
	SIM_Delay(10);

	char DataResponseStr[50];
	memset(DataResponseStr, 0, sizeof(DataResponseStr));

	int count = 0;
	for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
		if(_sim->buffer[i] == '\r') break;
		DataResponseStr[count] = _sim->buffer[i];
		++count;
	}
	int DataResponseLen = -1;

	int analysisHttpCode;

	sscanf(DataResponseStr, "+HTTPACTION: 0,%d,%d", &analysisHttpCode, &DataResponseLen);
	*httpCode = analysisHttpCode;

#ifdef SIM_Debug
	strcpy(_sim->DebugResponceStr, DataResponseStr);
	_sim->DebugHTTPGetCode = *httpCode;
	_sim->DebugHTTPGetDataLen = DataResponseLen;
#endif

	if(*httpCode == 200){
		ClearBuffer(_sim);
		char readDataStr[50];
		memset(readDataStr, 0, sizeof(readDataStr));

		sprintf(readDataStr,"AT+HTTPREAD=0,%d\r\n", DataResponseLen);

		foundRes = SendCommand(_sim, readDataStr, "+HTTPREAD:", 2000, 0);
		if(foundRes == -1){
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"Read HTTP data ERROR");
#endif
			ErrCheck = SIM7600_ERROR;
		}

		int countCheck = 0;
		while(1){
			if(WaitForResponse(_sim, "+HTTPREAD: 0", 1000, 0) != -1) break;
			else if(WaitForResponse(_sim, "+HTTPREAD:0", 1000, 0) != -1) break;
#ifdef SIM_Debug
			memset(_sim->response, 0, sizeof(_sim->response));
			strcpy(_sim->response,"Check HTTPREAD Again");
//			HAL_Delay(1000);
#endif
			++countCheck;
			if(countCheck > 14) {

				goto StopHttpGet;
//				break;
			}
		}

		for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
			if(_sim->buffer[i] == '\n'){
				foundRes = i + 1;
				break;
			}
		}
		count = 0;

		memset(response, 0, sizeof(*response));
		for(int i = foundRes; i <= SIM_RX_BUFF_SIZE; ++i){
			if(_sim->buffer[i] == '\r') break;
			response[count] = _sim->buffer[i];
			++count;
		}

		ClearBuffer(_sim);
	}
	else{
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"HTTP CODE RETURN ERROR");
#endif
	}

//	Stop HTTP request
	StopHttpGet:
	DeInitHTTP(_sim);

	if(ErrCheck == SIM7600_OK){
#ifdef SIM_Debug
		memset(_sim->response, 0, sizeof(_sim->response));
		strcpy(_sim->response,"HTTP GET OK");
		++_sim->CountHTTPGetOK;
#endif
	}
	return SuperviseHTTP(_sim, ErrCheck);
}




