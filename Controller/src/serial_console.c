#include "serial_console.h"
#include "commands.h"
#include "serial.h"
#include "log.h"
#include "buffers.h"

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static void echo_command(const char *arguments, uint8_t length, const command_functions_t* output);

static circular_buffer_t m_commandBuffer;
static uint8_t m_commandBufferData[64];
static bool m_echo = true;

static const command_functions_t m_output = {
  .write = log_writeln,
  .write_format = log_write_format,
  .writeln = log_writeln,
  .writeln_format = log_writeln_format
};

static const command_t m_echoCommand = {
  .prefix = "ECHO",
  .summary = "Enable or disable echo of input characters",
  .handler = echo_command
};

static bool is_allowed_command_char(uint8_t data);


void serial_console_initialize(void)
{
  circular_buffer_initialize(&m_commandBuffer, &m_commandBufferData[0], sizeof(m_commandBufferData));

  commands_register(&m_echoCommand);
}

void serial_console_poll(void)
{
  uint8_t data;
  if (serial_read_byte(&data))
  {
    if (false == is_allowed_command_char(data))
    {
      return;
    }

    bool output = true;
    bool handleCommand = false;

    if (data == 0x0D)
    {
      // Enter
      handleCommand = true;
      output = false;
      if (m_echo)
      {
        log_writeln("");
      }
    }
    else if (data == 0x7F)
    {
      // Backspace
      if (m_commandBuffer.count > 0)
      {
        m_commandBuffer.count--;
      }
      else
      {
        output = false;
      }
    }
    else
    {
      output = circular_buffer_write(&m_commandBuffer, &data, 1);
    }

    if (output && m_echo)
    {
      log_write_char(data);
    }

    if (handleCommand)
    {
      uint8_t length = m_commandBuffer.count;
      char data[sizeof(m_commandBufferData) + 1];
      memset(&data[0], 0, sizeof(data));

      if (false == circular_buffer_read(&m_commandBuffer, (uint8_t*)&data[0], length))
      {
        log_writeln(ERR_WITH_REASON(COM_ERR_UNKNOWN));
        return;
      }

      commands_handle(&data[0], length, &m_output);
    }
  }
}

static bool is_allowed_command_char(uint8_t data)
{
  return isprint(data) || (data == 0x0D) || (data == 0x7F);
}

static void echo_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  if (commands_match(arguments, length, "ON"))
  {
    m_echo = true;
  }
  else if (commands_match(arguments, length, "OFF"))
  {
    m_echo = false;
  }
  else
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
    return;
  }

  output->writeln(COM_OK);
}