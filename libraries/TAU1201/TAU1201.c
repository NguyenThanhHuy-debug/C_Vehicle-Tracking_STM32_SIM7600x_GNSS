/*
 * TAU1201.c
 *
 *  Created on: Oct 26, 2023
 *      Author: selina
 */

#include "TAU1201.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

extern UART_HandleTypeDef huart4;

#if (GPS_DEBUG == 1)
#include <usbd_cdc_if.h>
#endif

uint8_t rx_data = 0;
uint8_t rx_buffer[GPSBUFSIZE];
uint8_t rx_index = 0;

#if (GPS_DEBUG == 1)
void GPS_print(char *data){
	char buf[GPSBUFSIZE] = {0,};
	sprintf(buf, "%s\n", data);
	CDC_Transmit_FS((unsigned char *) buf, (uint16_t) strlen(buf));
}
#endif

#if (GPS_DEBUG == 2)
void GPS_print_uart(char *data){
	char buf[GPSBUFSIZE] = {0,};
	sprintf(buf, "%s\n", data);

	HAL_UART_Transmit(&huart4, (char*)buf, strlen(buf), 1);

//	CDC_Transmit_FS((unsigned char *) buf, (uint16_t) strlen(buf));
}
#endif
//
//// Recheck GPS module connect
//  if(gps.module_found == 0){
//	  gps.huart->Init.BaudRate = 115200;
//	  if (HAL_UART_Init(gps.huart) != HAL_OK)
//	  {
//	    Error_Handler();
//	  }
//	  GPS_Init(&gps, gps.huart);
//  }
int baud_num = -1;
int baud[9] = {4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

int auto_baud(TAU1201_t *_gps, int baudrate){
//	Check if this baudrate ok or not
	_gps->huart->Init.BaudRate = baudrate;
	if (HAL_UART_Init(_gps->huart) != HAL_OK){
		Error_Handler();
	}

	HAL_UART_Receive_IT(_gps->huart, &rx_data, 1);
	HAL_Delay(1000);
// If baudrate ok, module founded, return OK
	if(_gps->module_found == 1) return GPS_OK;
// Else check another baudrate
	++baud_num;
	if(baud_num > 8) return GPS_ERROR;
	return auto_baud(_gps, baud[baud_num]);
}

int GPS_Init(TAU1201_t *_gps, UART_HandleTypeDef *_huart){
	_gps->huart = _huart;
	_gps->module_found = 0;

	return auto_baud(_gps, _gps->huart->Init.BaudRate);
}

void GPS_UART_CallBack(TAU1201_t *_gps){
	if (rx_data != '\n' && rx_index < sizeof(rx_buffer)) {
		rx_buffer[rx_index++] = rx_data;
	}
	else {

		#if (GPS_DEBUG == 1)
		GPS_print((char*)rx_buffer);
		#endif



		if(GPS_validate(_gps, (char*) rx_buffer)){
#if (GPS_DEBUG == 2)
			GPS_print_uart((char*)rx_buffer);
#endif
			_gps->module_found = 1;
			GPS_parse(_gps, (char*) rx_buffer);
		}

		rx_index = 0;
		memset(rx_buffer, 0, sizeof(rx_buffer));
	}
	HAL_UART_Receive_IT(_gps->huart, &rx_data, 1);
}


int GPS_validate(TAU1201_t *_gps, char *nmeastr){
    char check[3];
    char checkcalcstr[3];
    int i;
    int calculated_check;

    i=0;
    calculated_check=0;

    // check to ensure that the string starts with a $
    if(nmeastr[i] == '$')
        i++;
    else
        return 0;

    //No NULL reached, 75 char largest possible NMEA message, no '*' reached
    while((nmeastr[i] != 0) && (nmeastr[i] != '*') && (i < 75)){
        calculated_check ^= nmeastr[i];// calculate the checksum
        i++;
    }

    if(i >= 75){
        return 0;// the string was too long so return an error
    }

    if (nmeastr[i] == '*'){
        check[0] = nmeastr[i+1];    //put hex chars in check string
        check[1] = nmeastr[i+2];
        check[2] = 0;
    }
    else
        return 0;// no checksum separator found there for invalid

    sprintf(checkcalcstr,"%02X",calculated_check);
    return((checkcalcstr[0] == check[0])
        && (checkcalcstr[1] == check[1])) ? 1 : 0 ;
	return 0;
}
//
void GPS_parse(TAU1201_t *_gps, char *GPSstrParse){
/*
 *
$GPGGA,085608.00,1050.68761,N,10647.77208,E,1,03,5.89,13.9,M,-2.2,M,,*7A
$GPGSA,A,3,12,25,29,,,,,,,,,,13.46,5.89,12.10*0B
$GPGSV,3,1,11,05,,,29,06,05,119,28,11,33,092,26,12,17,200,41*48
$GPGSV,3,2,11,13,,,26,15,,,21,20,20,045,22,23,,,17*4F
$GPGSV,3,3,11,24,,,34,25,14,229,27,29,51,304,32*7F
$GPGLL,1050.68761,N,10647.77208,E,085608.00,A,A*6E
$GPRMC,085609.00,A,1050.68754,N,10647.77215,E,0.580,,020624,,,A*73
$GPVTG,,T,,M,0.580,N,1.075,K,A*2D*/


//	09:06:45  $GPGGA,090645.00,1050.69077,N,10647.77953,E,1,09,0.86,14.6,M,-2.2,M,,*7B
//	09:06:45  $GPGSA,A,3,12,24,23,05,15,11,25,29,13,,,,1.73,0.86,1.50*06
//	09:06:45  $GPGSV,3,1,12,05,29,021,26,06,03,122,22,11,31,098,22,12,15,196,33*7F
//	09:06:45  $GPGSV,3,2,12,13,51,036,25,15,74,320,27,18,10,325,22,20,18,049,18*7F
//	09:06:45  $GPGSV,3,3,12,23,09,279,22,24,32,170,28,25,13,225,26,29,54,297,26*73
//	09:06:45  $GPGLL,1050.69077,N,10647.77953,E,090645.00,A,A*67
//	09:06:45  $GPRMC,090645.00,A,1050.69077,N,10647.77953,E,0.754,,020624,,,A*7A
//	09:06:45  $GPVTG,,T,,M,0.754,N,1.396,K,A*28

/* Command for NEO7, 8M modules*/
    if(!strncmp(GPSstrParse, "$GPGGA", 6)){
    	if (sscanf(GPSstrParse, "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c", &_gps->utc_time, &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->lock, &_gps->satelites, &_gps->hdop, &_gps->msl_altitude, &_gps->msl_units) >= 1){
    		_gps->dec_latitude = GPS_nmea_to_dec(_gps->nmea_latitude, _gps->ns);
    		_gps->dec_longitude = GPS_nmea_to_dec(_gps->nmea_longitude, _gps->ew);
    		return;
    	}
    }

    else if (!strncmp(GPSstrParse, "$GPRMC", 6)){
		char utc_uint;
    	if(sscanf(GPSstrParse, "$GPRMC,%f,%c,%f,%c,%f,%c,%f,%f,%d", &_gps->utc_time, &utc_uint, &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->speed_k, &_gps->course_d, &_gps->date) >= 1){
    			_gps->speed_km = (_gps->speed_k * 1.852);
    		return;
    	}
    }

    else if (!strncmp(GPSstrParse, "$GPGLL", 6)){
        if(sscanf(GPSstrParse, "$GPGLL,%f,%c,%f,%c,%f,%c", &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->utc_time, &_gps->gll_status) >= 1)
            return;
    }
//
//    else if (!strncmp(GPSstrParse, "$GPVTG", 6)){
//        if(sscanf(GPSstrParse, "$GPVTG,%f,%c,%f,%c,%f,%c,%f,%c", &_gps->course_t, &_gps->course_t_unit, &_gps->course_m, &_gps->course_m_unit, &_gps->speed_k, &_gps->speed_k_unit, &_gps->speed_km, &_gps->speed_km_unit) >= 1)
//            return;
//    }
//
//    else if (!strncmp(GPSstrParse, "$GPGLL", 6)){
//        if(sscanf(GPSstrParse, "$GPGLL,%f,%c,%f,%c,%f,%c", &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->utc_time, &_gps->gll_status) >= 1)
//            return;
//    }

    /* Command for TAU1201 module (NMEA V4.0/4.1)*/
    //	06:16:34  $GNGGA,061634.000,1050.68724,N,10647.78479,E,1,24,0.56,41.8,M,-1.2,M,,*58
    //	06:16:34  $GPGSA,A,3,05,19,13,194,15,06,17,25,22,,,,1.02,0.56,0.86,1*21
    //	06:16:34  $GAGSA,A,3,308,327,,,,,,,,,,,1.02,0.56,0.86,3*0F
    //	06:16:34  $BDGSA,A,3,207,242,221,213,238,201,214,244,206,224,209,243,1.02,0.56,0.86,4*04
    //	06:16:34  $BDGSA,A,3,235,,,,,,,,,,,,1.02,0.56,0.86,4*31

    //	06:16:34  $GPGSV,4,1,13,195,58,28,,5,55,285,34,19,40,114,24,12,40,276,24*45
    //	06:16:34  $GPGSV,4,2,13,13,37,167,27,194,37,154,38,844,37,154,19,15,21,202,40*7A
    //	06:16:34  $GPGSV,4,3,13,6,20,52,32,17,20,128,24,25,18,298,18,675,18,298,20*4F
    //	06:16:34  $GPGSV,4,4,13,22,10,161,35*4A
    //	06:16:34  $GAGSV,1,1,04,308,44,78,22,327,41,199,36,977,41,199,26,307,21,194,37*59

    //	06:16:34  $BDGSV,6,1,23,207,83,167,30,203,77,159,23,242,72,216,37,892,72,216,25*6A
    //	06:16:34  $BDGSV,6,2,23,221,51,34,33,621,51,34,19,213,51,316,29,208,49,334,*69
    //	06:16:34  $BDGSV,6,3,23,238,47,351,29,638,47,351,22,201,45,101,36,214,43,215,40*66
    //	06:16:34  $BDGSV,6,4,23,244,39,82,27,206,19,172,30,224,18,196,41,874,18,196,16*5A
    //	06:16:34  $BDGSV,6,5,23,624,18,196,36,209,15,186,35,243,13,324,24,235,5,115,35*58
    //	06:16:34  $BDGSV,6,6,23,635,5,115,24,226,2,150,36,626,2,150,31*5C

    //	06:16:34  $GNRMC,061634.000,A,1050.68724,N,10647.78479,E,0.001,23.42,050624,,,A*7F
    //	06:16:34  $GNZDA,061634.000,05,06,2024,00,00*49
    //	06:16:34  $GNTXT,01,01,01,ANT_OK*50

    	      //  $GPVTG,,T,,M,0.580,N,1.075,K,A*2D

    else if(!strncmp(GPSstrParse, "$GNGGA", 6)){
    	if (sscanf(GPSstrParse, "$GNGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c", &_gps->utc_time, &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->lock, &_gps->satelites, &_gps->hdop, &_gps->msl_altitude, &_gps->msl_units) >= 1){
    		_gps->dec_latitude = GPS_nmea_to_dec(_gps->nmea_latitude, _gps->ns);
    		_gps->dec_longitude = GPS_nmea_to_dec(_gps->nmea_longitude, _gps->ew);
    		return;
    	}
    }

    else if (!strncmp(GPSstrParse, "$GNGLL", 6)){
        if(sscanf(GPSstrParse, "$GNGLL,%f,%c,%f,%c,%f,%c", &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->utc_time, &_gps->gll_status) >= 1)
            return;
    }
	//	06:16:34  $GNRMC,061634.000,A,1050.68724,N,10647.78479,E,0.001,23.42,050624,,,A*7F
    //	09:06:45  $GPRMC,090645.00,A,1050.69077,N,10647.77953,E,0.754,,020624,,,A*7A

    else if (!strncmp(GPSstrParse, "$GNRMC", 6)){
		char utc_uint;
    	if(sscanf(GPSstrParse, "$GNRMC,%f,%c,%f,%c,%f,%c,%f,%f,%d", &_gps->utc_time, &utc_uint, &_gps->nmea_latitude, &_gps->ns, &_gps->nmea_longitude, &_gps->ew, &_gps->speed_k, &_gps->course_d, &_gps->date) >= 1){
    		_gps->speed_km = (_gps->speed_k * 1.852);
    		return;
    	}

    }




//    else if (!strncmp(GPSstrParse, "$GNRMC", 6)){

//        if(sscanf(GPSstrParse, "$GNRMC,%f,%c,%f", &_gps->utc_time, utc_uint,&_gps->nmea_latitude) >= 1){
//
//        }
////            return;
//    }
//    else if (!strncmp(GPSstrParse, "$GNVTG", 6)){
//        if(sscanf(GPSstrParse, "$GNVTG,%f,%c,%f,%c,%f,%c,%f,%c", &_gps->course_t, &_gps->course_t_unit, &_gps->course_m, &_gps->course_m_unit, &_gps->speed_k, &_gps->speed_k_unit, &_gps->speed_km, &_gps->speed_km_unit) >= 1)
//            return;
//    }


}

float GPS_nmea_to_dec(float deg_coord, char nsew){
    int degree = (int)(deg_coord/100);
    float minutes = deg_coord - degree*100;
    float dec_deg = minutes / 60;
    float decimal = degree + dec_deg;
    if (nsew == 'S' || nsew == 'W') { // return negative
        decimal *= -1;
    }
    return decimal;
}

float getLatitude(TAU1201_t *_gps){
	return _gps->dec_latitude;
}

float getLongitude(TAU1201_t *_gps){
	return _gps->dec_longitude;
}

float getSpeed(TAU1201_t *_gps){
	return _gps->speed_km;
}

float getSatelitesNum(TAU1201_t *_gps){
	return _gps->satelites;
}
