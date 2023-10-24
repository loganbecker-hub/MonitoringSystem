#ifndef KEYPAD_H
#define KEYPAD_H

#include "main.h"
#include <stdbool.h>

#define ROW_1_Pin GPIO_PIN_12
#define ROW_2_Pin GPIO_PIN_13
#define ROW_3_Pin GPIO_PIN_14
#define ROW_4_Pin GPIO_PIN_15

#define COL_1_Pin GPIO_PIN_4
#define COL_2_Pin GPIO_PIN_5
#define COL_3_Pin GPIO_PIN_6
#define COL_4_Pin GPIO_PIN_7

typedef enum
{
	KEY_TYPE_NONE 		= 0,
	KEY_TYPE_CANCEL 	= 1,
	KEY_TYPE_CONFIRM 	= 2,
}KEY_TYPE;

void keypad_init(void);
char keypad_scan(void);
char keypad_wait_for_key(uint32_t debounce_ms);
bool keypad_is_digit(char key);
KEY_TYPE keypad_confirm_or_cancel(char key);

#endif
