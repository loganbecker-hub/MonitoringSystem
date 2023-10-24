#ifndef LCD_H
#define LCD_H

#include <stdio.h>
#include <stdint.h>

#define RS		GPIO_PIN_3
#define EN		GPIO_PIN_4

#define	D4		GPIO_PIN_8
#define D5		GPIO_PIN_9
#define D6		GPIO_PIN_10
#define D7		GPIO_PIN_11

typedef struct
{
	char message[16];
	uint8_t row;
	uint8_t col;
}lcd_event_t;

/* Function declarations */
void Lcd_command_4bit(int8_t cmd);
void Lcd_init_4bit(void);
void Lcd_char_4bit(int8_t dat);
void Lcd_string_4bit(const char *msg);
void Lcd_string_xy(uint8_t row,uint8_t pos,const char *msg);
void Lcd_clear_4bit(void);
void Lcd_setCursor(int row, int column);

#endif // LCD_H
