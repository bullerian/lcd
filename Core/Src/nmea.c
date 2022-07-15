/*
 * nmea.c
 *
 *  Created on: 28 черв. 2022 р.
 *      Author: Professional
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "nmea.h"
#include "lcd.h"
#include "main.h"

#define LOCAL_OFFSET (3)
#define KNOTS_TO_KM (1.852)
#define VAL_BUFFER_SIZE 20

static char *parse_utc(char *start, utc_t *time);
static char *parse_coord(char* start, coord_t *coord, uint8_t is_longitude);
static void parse_speed(char* buffer, splitted_t *speed);
static void parse_date(char* start, date_t *date);
static int handle_rmc(char *start, rmc_t *data);
static int handle_gsv(char *start, gsv_t *data);
static int handle_gga(char *start, gga_t *data);
static int get_range(char *str, size_t index, char **start, size_t *size);
static int copy_value_to_buffer(char *str, size_t index);
static int crc_check(char *start);
static void float_to_ints(double val, splitted_t *out, uint8_t precission);

uint8_t example[] = "$GPRMC,171426.00,A,4951.99383,N,02402.47727,E,0.642,,060722,,,A*72\r\n"\
					"$GPVTG,,,,,,,,,N*30\r\n"\
					"$GPGGA,,,,,,0,00,99.99,,,,,,*48\r\n"\
					"$GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30\r\n"\
		            "$GPGSV,4,1,13,01,13,169,33,02,08,326,,03,55,110,29,04,85,054,08*76\r\n"\
					"$GPGSV,4,2,13,06,42,300,,07,07,193,10,09,57,249,19,11,10,322,*7A\r\n"\
				    "$GPGSV,4,3,13,16,03,105,,17,06,241,,19,14,260,18,26,14,079,*74\r\n"\
					"$GPGSV,4,4,13,31,17,040,*4B\r\n"
					"$GPGLL,,,,,,V,N*64";
static char buffer[VAL_BUFFER_SIZE] = {0};

enum RMC_INDEX {
	RMC_INDEX_UTC = 0,
	RMC_INDEX_STATUS,
	RMC_INDEX_LATITUDE,
	RMC_INDEX_NS_INDICATOR,
	RMC_INDEX_LONGITUDE,
	RMC_INDEX_EW_INDICATOR,
	RMC_INDEX_SPEED_OVER_GROUND,
	RMC_INDEX_COURSE_OVER_GROUND,
	RMC_INDEX_DATE,
	RMC_INDEX_MAGNETIC_VARIATION,
	RMC_INDEX_EAST_WEST_INDICATION,
	RMC_INDEX_MODE
};

enum GSV_INDEX {
	GSV_INDEX_NUM_OF_MSG = 0,
	GSV_INDEX_MSG_NUM,
	GSV_INDEX_SATELITES
};

enum GGA_INDEX {
	GGA_INDEX_UTC_TIME = 0,
	GGA_INDEX_LATITUDE,
	GGA_INDEX_NS_INDICATOR,
	GGA_INDEX_LONGITUDE,
	GGA_INDEX_EW_INDICATOR,
	GGA_INDEX_POS_FIX,
	GGA_INDEX_SATS_USED,
	GGA_INDEX_HDOP,
	GGA_INDEX_ALTITUDE
};

int parse_nmea(char *str, rmc_t *rmc, gsv_t *gsv, gga_t *gga)
{
	char *line_start;

	if ((line_start = strstr(str, "$GPRMC")) != NULL)
	{
		if (crc_check(line_start) != RC_OK || handle_rmc(line_start, rmc) != RC_OK) return RC_ERROR;
	}

	if ((line_start = strstr(str, "$GPGSV")) != NULL)
	{
		if (crc_check(line_start) != RC_OK || handle_gsv(line_start, gsv) != RC_OK) return RC_ERROR;
	}

	if ((line_start = strstr(str, "$GPGGA")) != NULL)
	{
		if (crc_check(line_start) != RC_OK || handle_gga(line_start, gga) != RC_OK) return RC_ERROR;
	}

	return RC_OK;
}

static int handle_rmc(char *start, rmc_t *data)
{
	if(copy_value_to_buffer(start, RMC_INDEX_STATUS) == RC_ERROR) return RC_ERROR;
	if (buffer[0] != 'A') return RC_ERROR;

	if(copy_value_to_buffer(start, RMC_INDEX_UTC) == RC_ERROR) return RC_ERROR;
	parse_utc(buffer, &data->time);

	if(copy_value_to_buffer(start, RMC_INDEX_LATITUDE) == RC_ERROR) return RC_ERROR;
	parse_coord(buffer, &data->latitude, false);

	if(copy_value_to_buffer(start, RMC_INDEX_NS_INDICATOR) == RC_ERROR) return RC_ERROR;
	data->latitude.direction = buffer[0];

	if(copy_value_to_buffer(start, RMC_INDEX_LONGITUDE) == RC_ERROR) return RC_ERROR;
	parse_coord(buffer, &data->longitude, true);

	if(copy_value_to_buffer(start, RMC_INDEX_EW_INDICATOR) == RC_ERROR) return RC_ERROR;
	data->longitude.direction = buffer[0];

	if(copy_value_to_buffer(start, RMC_INDEX_COURSE_OVER_GROUND) == RC_ERROR) return RC_ERROR;
	float_to_ints(atof(buffer), &data->course, 2);

	if(copy_value_to_buffer(start, RMC_INDEX_SPEED_OVER_GROUND) == RC_ERROR) return RC_ERROR;
	parse_speed(buffer, &data->speed);

	if(copy_value_to_buffer(start, RMC_INDEX_DATE) == RC_ERROR) return RC_ERROR;
	parse_date(buffer, &data->date);

	return RC_OK;
}

static int handle_gga(char *start, gga_t *data)
{
	if(copy_value_to_buffer(start, GGA_INDEX_POS_FIX) == RC_ERROR) return RC_ERROR;
	data->pos_fix = atoi(buffer);

	if(copy_value_to_buffer(start, GGA_INDEX_SATS_USED) == RC_ERROR) return RC_ERROR;
	data->sats_used = atoi(buffer);

	if(copy_value_to_buffer(start, GGA_INDEX_HDOP) == RC_ERROR) return RC_ERROR;
	float_to_ints(atof(buffer), &data->hdop, 2);

	if(copy_value_to_buffer(start, GGA_INDEX_ALTITUDE) == RC_ERROR) return RC_ERROR;
	float_to_ints(atof(buffer), &data->altitude, 2);

	return RC_OK;
}

static int handle_gsv(char *start, gsv_t *data)
{
	if(copy_value_to_buffer(start, GSV_INDEX_SATELITES) == RC_ERROR) return RC_ERROR;
	data->satelites_num = atoi(buffer);

	return RC_OK;
}

static int crc_check(char *start)
{
	char *asterisk;
	char tmp_crc[2];
	size_t len;
	uint8_t actual_crc, expected_crc;

	if (*start != '$' || ((asterisk = strchr(start, '*')) == NULL)) return RC_ERROR;
	actual_crc = *++start;
	len = asterisk - start;
	// get two bytes that represent one hex byte
	tmp_crc[0] = *(asterisk + 1);
	tmp_crc[1] = *(asterisk + 2);
	expected_crc = (uint8_t)strtol(tmp_crc, NULL, 16);

	while (--len)
	{
		actual_crc ^= *++start;
	}

	return (actual_crc == expected_crc) ? RC_OK : RC_ERROR;
}


static void float_to_ints(double val, splitted_t *out, uint8_t precission)
{
	out->val = val;
	out->high = (uint16_t) val;
	out->low = (uint16_t)((val - out->high) * (10 * precission));
}

static int get_range(char *str, size_t index, char **start, size_t *size)
{
	char *end, *tmp_start;
	tmp_start = str;

	do
	{
		tmp_start = strchr(tmp_start, ',');
		if (tmp_start == NULL) return RC_ERROR;
		tmp_start++;
	} while(index--);

	end = strchr(tmp_start, ',');
	*size = end - tmp_start;
	*start = tmp_start;

	return RC_OK;
}

static int copy_value_to_buffer(char *str, size_t index)
{
	char *start;
	size_t size;

	if (get_range(str, index, &start, &size) == RC_ERROR)
	{
		return RC_ERROR;
	}

	memset(buffer, 0, VAL_BUFFER_SIZE);
	memcpy(buffer, start, size);

	return RC_OK;
}

static char *parse_utc(char *start, utc_t *time)
{
	char tmp[4] = {0};

	tmp[0] = *start++;
	tmp[1] = *start++;

	time->hour = atoi(tmp);

	time->hour = (time->hour + LOCAL_OFFSET) % 24;

	tmp[0] = *start++;
	tmp[1] = *start++;

	time->minute = atoi(tmp);

	tmp[0] = *start++;
	tmp[1] = *start++;

	time->second = atoi(tmp);

	// skip the dot
	start++;
	tmp[0] = *start++;
	tmp[1] = *start++;
	tmp[2] = *start++;
	tmp[3] = *start;

	time->millisec = atoi(tmp);

	return start;
}

static char* parse_coord(char* start, coord_t *coord, uint8_t is_longitude)
{
	char tmp[5] = {0};

	tmp[0] = *start++;
	tmp[1] = *start++;
	if (is_longitude)
	{
		tmp[2] = *start++;
	}
	coord->degree = atoi(tmp);
	tmp[2] = 0;

	tmp[0] = *start++;
	tmp[1] = *start++;
	coord->minute = atoi(tmp);
	// skip dot
	start++;

	tmp[0] = *start++;
	tmp[1] = *start++;
	tmp[2] = *start++;
	tmp[3] = *start++;
	coord->seconds = atoi(tmp);
	start++;

	coord->direction = *start++;

	return start;
}

static void parse_speed(char* buffer, splitted_t *speed)
{
	double tmp = atof(buffer) * KNOTS_TO_KM;

	float_to_ints(tmp, speed, 2);
}

static void parse_date(char* start, date_t *date)
{
	char tmp[4] = {0};

	tmp[0] = *start++;
	tmp[1] = *start++;
	date->day = (uint8_t)atoi(tmp);

	tmp[0] = *start++;
	tmp[1] = *start++;
	date->month = (uint8_t)atoi(tmp);

	tmp[0] = *start++;
	tmp[1] = *start++;
	date->year = (uint8_t)atoi(tmp);

	if (date->year != 22) {
		lcd_printf(3, 0, start);
	}
}


