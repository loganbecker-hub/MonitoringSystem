#include "lcd.h"
#include "main.h"
#include "projdefs.h"
#include "FreeRTOS.h"
#include "task.h"

void Lcd_command_4bit(int8_t cmd)
{
  // Get higher nibble
  int8_t temp = (cmd & 0xF0) >> 4;
  // D4 
  if(temp & 1){ HAL_GPIO_WritePin(GPIOA, D4, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D4, 0);}
  // D5 
  if(temp & 2){ HAL_GPIO_WritePin(GPIOA, D5, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D5, 0);}
  // D6 
  if(temp & 4){ HAL_GPIO_WritePin(GPIOA, D6, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D6, 0);}
  // D7
  if(temp & 8){ HAL_GPIO_WritePin(GPIOA, D7, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D7, 0);}
  // RS and EN 
  HAL_GPIO_WritePin(GPIOA, RS, 0);
  HAL_GPIO_WritePin(GPIOA, EN, 1);
  __NOP();
  HAL_GPIO_WritePin(GPIOA, EN, 0);
  vTaskDelay(pdMS_TO_TICKS(1));

  // Get lower nibble
  temp = (cmd & 0x0F);
  // D4 
  if(temp & 1){ HAL_GPIO_WritePin(GPIOA, D4, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D4, 0);}
  // D5 
  if(temp & 2){ HAL_GPIO_WritePin(GPIOA, D5, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D5, 0);}
  // D6 
  if(temp & 4){ HAL_GPIO_WritePin(GPIOA, D6, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D6, 0);}
  // D7
  if(temp & 8){ HAL_GPIO_WritePin(GPIOA, D7, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D7, 0);}
  // RS and En 
  HAL_GPIO_WritePin(GPIOA, EN, 1);
  __NOP();
  HAL_GPIO_WritePin(GPIOA, EN, 0);
  vTaskDelay(pdMS_TO_TICKS(3));
}

void Lcd_init_4bit(void)
{       
	// Data Lines D4, D5, D6, D7
	GPIO_InitTypeDef pins;

	// RS - PA3
	pins.Pin = RS;
	pins.Mode = GPIO_MODE_OUTPUT_PP;
	pins.Pull = GPIO_NOPULL;
	pins.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &pins);

	// E - PA4
	pins.Pin = EN;
	pins.Mode = GPIO_MODE_OUTPUT_PP;
	pins.Pull = GPIO_NOPULL;
	pins.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &pins);

	// D4 - PA8
	pins.Pin = D4;
	pins.Mode = GPIO_MODE_OUTPUT_PP;
	pins.Pull = GPIO_NOPULL;
	pins.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &pins);

	// D5 - PA9
	pins.Pin = D5;
	pins.Mode = GPIO_MODE_OUTPUT_PP;
	pins.Pull = GPIO_NOPULL;
	pins.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &pins);

	// D6 - PA10
	pins.Pin = D6;
	pins.Mode = GPIO_MODE_OUTPUT_PP;
	pins.Pull = GPIO_NOPULL;
	pins.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &pins);

	// D7 - PA11
	pins.Pin = D7;
	pins.Mode = GPIO_MODE_OUTPUT_PP;
	pins.Pull = GPIO_NOPULL;
	pins.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &pins);


	// Clear all the Pins
	HAL_GPIO_WritePin(GPIOA, RS, 0);
	HAL_GPIO_WritePin(GPIOA, EN, 0);

	HAL_GPIO_WritePin(GPIOA, D4, 0);
	HAL_GPIO_WritePin(GPIOA, D5, 0);
	HAL_GPIO_WritePin(GPIOA, D6, 0);
	HAL_GPIO_WritePin(GPIOA, D7, 0);

	vTaskDelay(pdMS_TO_TICKS(15));
	Lcd_command_4bit(0x02); // Send for initialization of LCD with nibble method
	Lcd_command_4bit(0x28); // Use 2 line and initialize 5*7 matrix in (4-bit mode)
	Lcd_command_4bit(0x01); // Clear display screen
	Lcd_command_4bit(0x0c); // Display on and cursor off
	Lcd_command_4bit(0x06); // Increment cursor (shift cursor to right)
}

void Lcd_char_4bit(int8_t dat)
{
  // Get higher nibble
  int8_t temp = (dat & 0xF0) >> 4;
  // D4 
  if(temp & 1){ HAL_GPIO_WritePin(GPIOA, D4, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D4, 0);}
  // D5 
  if(temp & 2){ HAL_GPIO_WritePin(GPIOA, D5, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D5, 0);}
  // D6 
  if(temp & 4){ HAL_GPIO_WritePin(GPIOA, D6, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D6, 0);}
  // D7
  if(temp & 8){ HAL_GPIO_WritePin(GPIOA, D7, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D7, 0);}
  // RS and EN 
  HAL_GPIO_WritePin(GPIOA,	RS, 1);
  HAL_GPIO_WritePin(GPIOA, EN, 1);
  __NOP();
  HAL_GPIO_WritePin(GPIOA, EN, 0);
  vTaskDelay(pdMS_TO_TICKS(1));

  // Get lower nibble
  temp = (dat & 0x0F);
  // D4 
  if(temp & 1){ HAL_GPIO_WritePin(GPIOA, D4, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D4, 0);}
  // D5 
  if(temp & 2){ HAL_GPIO_WritePin(GPIOA, D5, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D5, 0);}
  // D6 
  if(temp & 4){ HAL_GPIO_WritePin(GPIOA, D6, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D6, 0);}
  // D7
  if(temp & 8){ HAL_GPIO_WritePin(GPIOA, D7, 1);}
  else{ HAL_GPIO_WritePin(GPIOA, D7, 0);}
  // RS and En 
  HAL_GPIO_WritePin(GPIOA, EN, 1);
  __NOP();
  HAL_GPIO_WritePin(GPIOA, EN, 0);
  vTaskDelay(pdMS_TO_TICKS(3));
}

void Lcd_string_4bit(const char *msg)
{
  while((*msg)!= 0)
  {		
    Lcd_char_4bit(*msg);
	msg++;	
  }
}

void Lcd_string_xy(uint8_t row,uint8_t pos,const char *msg)
{
  char location=0;
  
  if(row <= 1)
  {
    location = (0x80) | ( (pos) & 0x0f ); // Print message on 1st row and desired location
    Lcd_command_4bit(location);
  }
  else
  {
    location = (0xC0) | ( (pos) & 0x0f); // Print message on 2nd row and desired location
    Lcd_command_4bit(location);    
  }  
  
  Lcd_string_4bit(msg);
}

void Lcd_clear_4bit(void)
{
  Lcd_command_4bit(0x01);
}

void Lcd_setCursor(int row, int column)
{
    // Display, Cursor and CursorBlink On
    Lcd_command_4bit(0x0F);

    // Cursor movement
    uint8_t choice = 0x00;
    choice |= 1U<<4;

    choice |= (1U<<3);
    choice &= ~(1U<<2);
    Lcd_command_4bit(choice);
}
