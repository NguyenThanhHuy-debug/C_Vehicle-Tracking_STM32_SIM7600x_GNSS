/*
 * ota.h
 *
 *  Created on: Aug 2, 2024
 *      Author: selina
 */

#ifndef INC_OTA_H_
#define INC_OTA_H_

#include "main.h"

#define FIRMWARE_FILE_LINK 					"firmware.bin"


#define OTA_REQUEST							2
#define NO_OTA								0
#define OTA_COMPLETED						1
#define ERR_NO_FIRMWARE_FILE_FOUND			-1

int UpdateFirmwareFromURL(char *url);


#endif /* INC_OTA_H_ */
