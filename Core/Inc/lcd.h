/*
 * lcd.h
 *
 *  Created on: 24 черв. 2022 р.
 *      Author: Professional
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "stm32f4xx_hal.h"

void init(void);
int lcd_print(uint8_t row, uint8_t col, char* text);
void lcd_printf(uint8_t row, uint8_t col, const char* fmt, ...);
void lcd_clear(void);


#endif /* INC_LCD_H_ */
