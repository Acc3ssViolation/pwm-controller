#include "arduino/lcd1602.h"
#include "arduino/mega.h"
#include <stdbool.h>
#include <util/delay.h>
#include <string.h>

#define LCD_COMMAND_DISPLAY     0x08
#define LCD_COMMAND_CLEAR       0x01
#define LCD_COMMAND_DATA_ADDR   0x80

static void set_data_pins_as_output(lcd1602_t* lcd);

static void set_data_pins_as_input(lcd1602_t* lcd);

static void set_data_pins(lcd1602_t* lcd, uint8_t value);

static void send_data(lcd1602_t* lcd, uint8_t data);

static void send_command(lcd1602_t* lcd, uint8_t command);

static void send_command_no_wait(lcd1602_t* lcd, uint8_t command);

static void wait_for_busy_clear(lcd1602_t* lcd);

static void send_byte(lcd1602_t* lcd, uint8_t byte, bool registerSelect);

void lcd1602_initialize(lcd1602_t* lcd, uint8_t width, uint8_t height, const lcd_pins_t* pins)
{
  memcpy(&lcd->pins, pins, sizeof(lcd_pins_t));
  lcd->width = width;
  lcd->height = height;

  gpio_configure_output(lcd->pins.enable.port, lcd->pins.enable.pin);
  gpio_configure_output(lcd->pins.readWrite.port, lcd->pins.readWrite.pin);
  gpio_configure_output(lcd->pins.registerSelect.port, lcd->pins.registerSelect.pin);
  set_data_pins_as_input(lcd);

  // Init sequence
  _delay_ms(100);

  send_command_no_wait(lcd, 0x38);

  _delay_us(4500);

  send_command_no_wait(lcd, 0x38);

  _delay_us(150);

  send_command_no_wait(lcd, 0x38);

  // 4 bit, 2 lines, 5x7 dots
  send_command(lcd, 0x28);

  // Display off
  send_command(lcd, LCD_COMMAND_DISPLAY);

  // Display clear
  send_command(lcd, LCD_COMMAND_CLEAR);

  // Entry mode
  send_command(lcd, 0x06);

  wait_for_busy_clear(lcd);
}

void lcd1602_enable_cursor(lcd1602_t* lcd)
{
  lcd->displayOptions |= LCD_ENABLE_CURSOR;
  send_command(lcd, LCD_COMMAND_DISPLAY | lcd->displayOptions);
}

void lcd1602_disable_cursor(lcd1602_t* lcd)
{
  lcd->displayOptions &= ~LCD_ENABLE_CURSOR;
  send_command(lcd, LCD_COMMAND_DISPLAY | lcd->displayOptions);
}

void lcd1602_enable_display(lcd1602_t* lcd)
{
  lcd->displayOptions |= LCD_ENABLE_DISPLAY;
  send_command(lcd, LCD_COMMAND_DISPLAY | lcd->displayOptions);
}

void lcd1602_disable_display(lcd1602_t* lcd)
{
  lcd->displayOptions &= ~LCD_ENABLE_DISPLAY;
  send_command(lcd, LCD_COMMAND_DISPLAY | lcd->displayOptions);
}

void lcd1602_enable_blink(lcd1602_t* lcd)
{
  lcd->displayOptions |= LCD_ENABLE_BLINK;
  send_command(lcd, LCD_COMMAND_DISPLAY | lcd->displayOptions);
}

void lcd1602_disable_blink(lcd1602_t* lcd)
{
  lcd->displayOptions &= ~LCD_ENABLE_BLINK;
  send_command(lcd, LCD_COMMAND_DISPLAY | lcd->displayOptions);
}

void lcd1602_clear(lcd1602_t* lcd)
{
  send_command(lcd, LCD_COMMAND_CLEAR);
}

void lcd1602_print(lcd1602_t* lcd, const char* string)
{
  size_t length = strlen(string);
  for (size_t i = 0; i < length; i++)
  {
    send_data(lcd, string[i]);
  }
}

void lcd1602_set_cursor(lcd1602_t* lcd, uint8_t x, uint8_t y)
{
  // Position to address
  uint8_t address = 0;

  if (y == 0)
  address = 0;
  else if (y == 1)
  address = 0x40;
  else if (y == 2)
  address = lcd->width;
  else if (y == 3)
  address = 0x40 + lcd->width;

  address += x;

  send_command(lcd, LCD_COMMAND_DATA_ADDR | address);
}

static void send_data(lcd1602_t* lcd, uint8_t data)
{
  send_byte(lcd, data, true);
}

static void send_command(lcd1602_t* lcd, uint8_t command)
{
  wait_for_busy_clear(lcd);
  send_command_no_wait(lcd, command);
}

static void send_command_no_wait(lcd1602_t* lcd, uint8_t command)
{
  send_byte(lcd, command, false);
}

static void send_byte(lcd1602_t* lcd, uint8_t byte, bool registerSelect)
{
  // Set data pins
  set_data_pins_as_output(lcd);

  // Select register
  if (registerSelect)
  {
    gpio_set_pin(lcd->pins.registerSelect.port, lcd->pins.registerSelect.pin);
  }
  else
  {
    gpio_reset_pin(lcd->pins.registerSelect.port, lcd->pins.registerSelect.pin);
  }

  // Write mode
  gpio_reset_pin(lcd->pins.readWrite.port, lcd->pins.readWrite.pin);

  // Set high nibble
  set_data_pins(lcd, byte >> 4);

  // Clock data
  gpio_set_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
  _delay_us(2);
  gpio_reset_pin(lcd->pins.enable.port, lcd->pins.enable.pin);

  // Set low nibble
  set_data_pins(lcd, byte);

  // Clock data
  gpio_set_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
  _delay_us(2);
  gpio_reset_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
}

static void wait_for_busy_clear(lcd1602_t* lcd)
{
  bool isFlagSet;

  set_data_pins_as_input(lcd);
  gpio_set_pin(lcd->pins.readWrite.port, lcd->pins.readWrite.pin);
  gpio_reset_pin(lcd->pins.registerSelect.port, lcd->pins.registerSelect.pin);

  do
  {
    // Clock data
    gpio_set_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
    _delay_us(2);
    isFlagSet = gpio_get_input(lcd->pins.data[3].port, lcd->pins.data[3].pin);
    gpio_reset_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
    // Read and discard lower nibble
    _delay_us(2);
    gpio_set_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
    _delay_us(2);
    gpio_reset_pin(lcd->pins.enable.port, lcd->pins.enable.pin);
  }
  while (isFlagSet);
}

static void set_data_pins_as_output(lcd1602_t* lcd)
{
  for (uint8_t i = 0; i < 4; i++)
  {
    gpio_configure_output(lcd->pins.data[i].port, lcd->pins.data[i].pin);
  }
}

static void set_data_pins_as_input(lcd1602_t* lcd)
{
  for (uint8_t i = 0; i < 4; i++)
  {
    gpio_configure_input(lcd->pins.data[i].port, lcd->pins.data[i].pin);
  }
}

static void set_data_pins(lcd1602_t* lcd, uint8_t value)
{
  for (uint8_t i = 0; i < 4; i++)
  {
    if (value & (1 << i))
    {
      gpio_set_pin(lcd->pins.data[i].port, lcd->pins.data[i].pin);
    }
    else
    {
      gpio_reset_pin(lcd->pins.data[i].port, lcd->pins.data[i].pin);
    }
  }
}