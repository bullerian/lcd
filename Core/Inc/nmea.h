/*
 * nmea.h
 *
 *  Created on: 28 черв. 2022 р.
 *      Author: Professional
 */

#ifndef INC_NMEA_H_
#define INC_NMEA_H_

enum HEADING {
	N = 'N',
	S = 'S',
	E = 'E',
	W = 'W'
};

typedef struct {
	double val;
	uint16_t high;
	uint16_t low;
} splitted_t;

typedef struct {
	int8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t millisec;
} utc_t;

typedef struct {
	uint8_t direction;
	uint16_t degree;
	uint8_t minute;
	uint32_t seconds;
} coord_t;

typedef struct {
	uint8_t day;
	uint8_t month;
	uint8_t year;
} date_t;

typedef struct {
	utc_t time;
	uint8_t status;
	coord_t latitude;
	coord_t longitude;
	splitted_t speed;
	splitted_t course;
	date_t date;
} rmc_t;

typedef struct {
	uint8_t id;
	uint8_t elevation;
	uint16_t azimuth;
	uint8_t snr;
} satelite_t;

typedef struct {
	uint8_t satelites_num;
	struct node *list;
} gsv_t;

typedef struct {
	uint8_t pos_fix;
	uint8_t sats_used;
	splitted_t hdop;
	splitted_t altitude;
} gga_t;

int parse_nmea(char *str, rmc_t *rmc, gsv_t *gsv, gga_t *gga);


#endif /* INC_NMEA_H_ */
