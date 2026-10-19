/*
 * ota.c
 *
 *  Created on: Aug 2, 2024
 *      Author: selina
 */

#include "ota.h"

extern sim7600_t sim;
extern server_t _Server;
extern board_parameter_t BoardParameter;
extern eeprom_24LCxx_t eeprom;

int UpdateFirmwareFromURL(char *url){
	int ErrCheck = SIM7600_OK;
	printf("Starting OTA\r\n");
	ErrCheck = DownloadFile2(&sim, url, FIRMWARE_FILE_LINK, &_Server.HTTPGetCode);
	if(ErrCheck == SIM7600_OK){
		printf("Firmware file download completed, reset MCU\r\n");
		BoardParameter.OTA_Flag = OTA_REQUEST;
		EEPROM_write_bytes(&eeprom, 0, (uint8_t *)&BoardParameter, sizeof(BoardParameter));

		return SIM7600_OK;
	}

	return SIM7600_ERROR;
}


