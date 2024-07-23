#ifndef LCD1602_H_
#define LCD1602_H_

#include "arduino/gpio.h"

#define LCD_ENABLE_DISPLAY    0x04
#define LCD_ENABLE_CURSOR     0x02
#define LCD_ENABLE_BLINK      0x01

typedef struct 
{
  gpio_info_t registerSelect;
  gpio_info_t readWrite;
  gpio_info_t enable;
  gpio_info_t data[4];
} lcd_pins_t;

typedef struct  
{
  lcd_pins_t pins;
  uint8_t displayOptions;
  uint8_t width;
  uint8_t height;
} lcd1602_t;

void lcd1602_initialize(lcd1602_t* lcd, uint8_t width, uint8_t height, const lcd_pins_t* pins);

void lcd1602_enable_cursor(lcd1602_t* lcd);

void lcd1602_disable_cursor(lcd1602_t* lcd);

void lcd1602_enable_display(lcd1602_t* lcd);

void lcd1602_disable_display(lcd1602_t* lcd);

void lcd1602_enable_blink(lcd1602_t* lcd);

void lcd1602_disable_blink(lcd1602_t* lcd);

void lcd1602_clear(lcd1602_t* lcd);

void lcd1602_print(lcd1602_t* lcd, const char* string);

void lcd1602_set_cursor(lcd1602_t* lcd, uint8_t x, uint8_t y);

#endif /* LCD1602_H_ */