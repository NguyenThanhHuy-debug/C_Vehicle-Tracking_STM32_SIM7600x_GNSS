/*
 * TAU1201.h
 *
 *  Created on: Oct 26, 2023
 *      Author: selina
 */

#ifndef INC_TAU1201_H_
#define INC_TAU1201_H_

#include "main.h"

#define GPS_DEBUG	0
#define GPSBUFSIZE  128       // GPS buffer size

#define GPS_OK		1
#define GPS_ERROR	-1

typedef struct{
	UART_HandleTypeDef *huart;


	uint8_t module_found;

    // calculated values
    float dec_longitude;
    float dec_latitude;
    float altitude_ft;

    // GGA - Global Positioning System Fixed Data
    float nmea_longitude;
    float nmea_latitude;
    float utc_time;
    char ns, ew;
    int lock;
    int satelites;
    float hdop;
    float msl_altitude;
    char msl_units;

    // RMC - Recommended Minimmum Specific GNS Data
    char rmc_status;
    float speed_k;
    float course_d;
    int date;
    // GLL
    char gll_status;
    // VTG - Course over ground, ground speed
    float course_t; // ground speed true
    char course_t_unit;
    float course_m; // magnetic
    char course_m_unit;
    char speed_k_unit;
    float speed_km; // speek km/hr
    char speed_km_unit;
} TAU1201_t;

int GPS_Init(TAU1201_t *_gps, UART_HandleTypeDef *_huart);
void GPS_UART_CallBack(TAU1201_t *_gps);

int GPS_validate(TAU1201_t *_gps, char *nmeastr);
void GPS_parse(TAU1201_t *_gps, char *GPSstrParse);

float GPS_nmea_to_dec(float deg_coord, char nsew);

// Get Latitude and Longitude's function
float getLatitude(TAU1201_t *_gps);
float getLongitude(TAU1201_t *_gps);
// Get GPS module's moving speed
float getSpeed(TAU1201_t *_gps);

float getSatelitesNum(TAU1201_t *_gps);

#endif /* INC_TAU1201_H_ */
