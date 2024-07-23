#include "sysb/log.h"
#include "arduino/serial.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char printBuffer[200];

void log_initialize(void)
{
  serial_initialize();
}

void log_writeln(const char* string)
{
  uint8_t length = strnlen(string, UINT8_MAX);
  serial_send((const uint8_t*)string, length);
  // CR LF
  serial_send_byte(0x0D);
  serial_send_byte(0x0A);
}

void log_writeln_format(const char* string, ...)
{
  va_list args;
  va_start(args, string);
  vsnprintf(printBuffer, sizeof(printBuffer), string, args);
  va_end(args); 

  log_writeln(printBuffer);
}

void log_write_format(const char* string, ...)
{
  va_list args;
  va_start(args, string);
  vsnprintf(printBuffer, sizeof(printBuffer), string, args);
  va_end(args);
  uint8_t length = strnlen(printBuffer, UINT8_MAX);
  serial_send((const uint8_t*)printBuffer, length);
}

void log_write_char(const char chr)
{
  serial_send_byte((uint8_t) chr);
}