/*
 * lcd.c
 *
 *  Created on: 24 черв. 2022 р.
 *      Author: Professional
 */
#include <stdarg.h>
#include "lcd.h"


#define LCD_PORT GPIOB
#define E_PIN GPIO_PIN_8
#define RS_PIN GPIO_PIN_9

#define CURSOR (0x80)
#define CMD_CLEAR (0x1)
#define CHARS_IN_ROW (20)



void send_data(uint8_t data);
void send_cmd(uint8_t cmd);
void pulseEnable(void);
void set_cursor(uint8_t val);


void lcd_printf(uint8_t row, uint8_t col, const char* fmt, ...)
{
	char buff[20 * 4 + 1] = {0};

	va_list args;
	va_start(args, fmt);
	vsprintf(buff, fmt, args);
	va_end (args);

	lcd_print(row, col, buff);
}

int lcd_print(uint8_t row, uint8_t col, char* text)
{
	uint8_t address;
	uint8_t base = 0;

	if (col >= CHARS_IN_ROW ) return -1;

	switch (row)
	{
	case 0:
		base = 0;
		break;
	case 1:
		base = 64;
		break;
	case 2:
		base = 20;
		break;
	case 3:
		base = 84;
		break;
	default:
		return -1;
	}

	address = base + col;
	set_cursor(address);

	while (*text)
	{
		send_data(*text);
		text++;
		address++;
		if ((address - base) >= 19)
		{
			switch (address)
			{
			case 20:
				address = 64;
				break;
			case 84:
				address = 20;
				break;
			case 40:
				address = 84;
				break;
			case 104:
				address = 0;
				break;
			}
			set_cursor(address);
		}
	}

	return 0;
}

void pulseEnable(void)
{
	HAL_GPIO_WritePin(LCD_PORT, E_PIN, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(LCD_PORT, E_PIN, GPIO_PIN_RESET);
	HAL_Delay(1);
}

void send_data(uint8_t data)
{
	HAL_GPIO_WritePin(LCD_PORT, RS_PIN, GPIO_PIN_SET);
	LCD_PORT->ODR = (LCD_PORT->ODR & 0xFF00) | data;
	pulseEnable();
//	HAL_Delay(1);
}

void send_cmd(uint8_t cmd)
{
	HAL_GPIO_WritePin(LCD_PORT, RS_PIN, GPIO_PIN_RESET);
	LCD_PORT->ODR = (LCD_PORT->ODR & 0xFF00) | cmd;
	pulseEnable();
//	HAL_Delay(1);
}

void set_cursor(uint8_t val)
{
	send_cmd(CURSOR | val);
}

void lcd_clear(void)
{
	send_cmd(CMD_CLEAR);
}

void init(void)
{
	uint8_t cmd;
	HAL_Delay(100);
	send_cmd(CMD_CLEAR);
	send_cmd(0b10);
	send_cmd(0b1111);
	send_cmd(0b111000);
}
