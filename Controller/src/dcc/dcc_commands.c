#include "dcc/dcc_commands.h"
#include "dcc/dcc.h"
#include "dcc/dcc_service_mode.h"
#include "sysb/commands.h"
#include "sysb/timer.h"
#include "arduino/mega.h"
#include <string.h>
#include <ctype.h>

#define MODE_OPERATION    "OPERATION"
#define MODE_SERVICE      "SERVICE"
#define MODE_OFF          "OFF"

static void on_timer(uint8_t timer);
static void mode_command(const char *arguments, uint8_t length, const command_functions_t* output);
static void send_command(const char *arguments, uint8_t length, const command_functions_t* output);
static void set_cv_command(const char *arguments, uint8_t length, const command_functions_t* output);
static void verify_cv_command(const char *arguments, uint8_t length, const command_functions_t* output);
static void set_cv_bit_command(const char *arguments, uint8_t length, const command_functions_t* output);
static void verify_cv_bit_command(const char *arguments, uint8_t length, const command_functions_t* output);

static const command_t m_modeCommand = {
  .prefix = "DCC+M",
  .summary = "Sets the DCC mode (OPERATION, SERVICE, OFF). Omit the argument to get the current mode.",
  .handler = mode_command
};

static const command_t m_sendCommand = {
  .prefix = "DCC+S",
  .summary = "Sends a DCC packet (HEXSTRING)",
  .handler = send_command
};

static const command_t m_setCVCommand = {
  .prefix = "DCC+CV+W",
  .summary = "Set a CV to a given value (LOCADDR CVADDR VALUE)",
  .handler = set_cv_command
};

static const command_t m_verifyCVCommand = {
  .prefix = "DCC+CV+V",
  .summary = "Verify that a CV is set to a given value (LOCADDR CVADDR VALUE)",
  .handler = verify_cv_command
};

static const command_t m_setCVBitCommand = {
  .prefix = "DCC+CV+WB",
  .summary = "Set a bit in a CV to a given value (LOCADDR CVADDR BIT VALUE)",
  .handler = set_cv_bit_command
};

static const command_t m_verifyCVBitCommand = {
  .prefix = "DCC+CV+VB",
  .summary = "Verify that a CV bit is set to a given value (LOCADDR CVADDR BIT VALUE)",
  .handler = verify_cv_bit_command
};

static uint8_t m_blinkTimer;

void dcc_commands_initialize(void)
{
  commands_register(&m_modeCommand);
  commands_register(&m_sendCommand);
  commands_register(&m_setCVCommand);
  commands_register(&m_verifyCVCommand);
  commands_register(&m_setCVBitCommand);
  commands_register(&m_verifyCVBitCommand);

  m_blinkTimer = timer_create(TIMER_MODE_REPEATING, on_timer);

  timer_start(m_blinkTimer, 1000);
}

static void mode_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  if (commands_match(arguments, length, MODE_OPERATION))
  {
    if (!dcc_is_started() || (dcc_get_mode() != DCC_MODE_OPERATION))
    {
      dcc_stop();
      dcc_start(DCC_MODE_OPERATION);

      timer_start(m_blinkTimer, 200);
    }
    output->writeln(COM_OK);
  }
  else if (commands_match(arguments, length, MODE_SERVICE))
  {
    if (!dcc_is_started() || (dcc_get_mode() != DCC_MODE_SERVICE))
    {
      dcc_stop();
      dcc_start(DCC_MODE_SERVICE);

      timer_start(m_blinkTimer, 100);
    }
    output->writeln(COM_OK);
  }
  else if (commands_match(arguments, length, MODE_OFF))
  {
    if (dcc_is_started())
    {
      dcc_stop();

      timer_start(m_blinkTimer, 1000);
    }
    output->writeln(COM_OK);
  }
  else if (length == 0)
  {
    uint8_t enabled = dcc_is_started();
    dcc_mode_t mode = dcc_get_mode();
    const char *result = MODE_OFF;
    if (enabled)
    {
      if (mode == DCC_MODE_OPERATION)
      {
        result = MODE_OPERATION;
      }
      else
      {
        result = MODE_SERVICE;
      }
    }

    output->writeln_format(OK_WITH_RESULT("%s"), result);
  }
  else
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
    return;
  }
}

static bool getHexNibble(char chr, uint8_t *data)
{
  chr = tolower(chr);
  if (chr >= 'a' && chr <= 'f')
  {
    *data = chr - 'a' + 0xa;
    return true;
  }
  else if (chr >= '0' && chr <= '9')
  {
    *data = chr - '0';
    return true;
  }
  return false;
}

static void send_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  uint8_t commandBytes[20];
  uint8_t nrOfCommandBytes = 0;

  if (((length & 1) != 0) || length == 0)
  {
    // Uneven amount of chars, not allowed
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_SIZE));
    return;
  }

  for (uint8_t i = 0; i < (length - 1); i++)
  {
    uint8_t tempHigh;
    uint8_t tempLow;

    if (!isalnum(arguments[i]) || !getHexNibble(arguments[i], &tempHigh))
    {
      output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
      return;
    }
    i++;
    if (!isalnum(arguments[i]) || !getHexNibble(arguments[i], &tempLow))
    {
      output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
      return;
    }

    if (nrOfCommandBytes < sizeof(commandBytes))
    {
      commandBytes[nrOfCommandBytes] = (tempHigh << 4) | tempLow;
      nrOfCommandBytes++;
    }
    else
    {
      output->writeln(ERR_WITH_REASON(COM_DCC_ERR_SIZE));
      return;
    }
  }

  // Queue it
  uint8_t assignedId;
  if (dcc_queue_data(&commandBytes[0], nrOfCommandBytes, &assignedId))
  {
    output->writeln(COM_OK "+");
    output->writeln_format("ID %u+", assignedId);

    // Show parsed data
    for (uint8_t i = 0; i < nrOfCommandBytes; i++)
    {
      output->write_format("%02X", commandBytes[i]);
    }
    output->writeln("");
  }
  else
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
  }
}

static void on_service_mode_result(dcc_result_t result, void* userData)
{
  const command_functions_t* output = (const command_functions_t*)userData;

  switch (result)
  {
    case DCC_RESULT_ACK:
    {
      output->writeln(OK_WITH_RESULT("ACK"));
      break;
    }
    case DCC_RESULT_NACK:
    {
      output->writeln(OK_WITH_RESULT("NO ACK"));
      break;
    }
    case DCC_RESULT_TIMEOUT:
    {
      output->writeln(ERR_WITH_REASON("TIMEOUT"));
      break;
    }
  }
}

static void set_cv_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  uint16_t address;
  uint16_t cv;
  uint8_t data;

  if (!commands_get_u16(arguments, length, 0, &address) || !commands_get_u16(arguments, length, 1, &cv) || !commands_get_u8(arguments, length, 2, &data))
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
    return;
  }

  if (!dcc_is_started())
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
    return;
  }

  dcc_mode_t mode = dcc_get_mode();
  // Address is zero based, cv IDs are 1 based
  uint16_t cvAddress = cv - 1;

  switch (mode)
  {
    case DCC_MODE_OPERATION:
    {
      // Use an operation mode command sequence
      output->writeln(COM_ERR);
      return;
    }
    case DCC_MODE_SERVICE:
    {
      // Use the service mode command sequence with acknowledgment
      if (!dcc_service_mode_set_cv(cvAddress, data, on_service_mode_result, (void*)output))
      {
        // Unable to start CV configuration process, return error
        output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
      }
      return;
    }
  }
}

static void verify_cv_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  uint16_t address;
  uint16_t cv;
  uint8_t data;

  if (!commands_get_u16(arguments, length, 0, &address) || !commands_get_u16(arguments, length, 1, &cv) || !commands_get_u8(arguments, length, 2, &data))
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
    return;
  }

  if (!dcc_is_started())
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
    return;
  }

  dcc_mode_t mode = dcc_get_mode();
  // Address is zero based, cv IDs are 1 based
  uint16_t cvAddress = cv - 1;

  switch (mode)
  {
    case DCC_MODE_OPERATION:
    {
      // Use an operation mode command sequence
      output->writeln(COM_ERR);
      return;
    }
    case DCC_MODE_SERVICE:
    {
      // Use the service mode command sequence with acknowledgment
      if (!dcc_service_mode_verify_cv(cvAddress, data, on_service_mode_result, (void*)output))
      {
        // Unable to start CV configuration process, return error
        output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
      }
      return;
    }
  }
}

static void set_cv_bit_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  uint16_t address;
  uint16_t cv;
  uint8_t bit;
  uint8_t data;

  if (!commands_get_u16(arguments, length, 0, &address) || !commands_get_u16(arguments, length, 1, &cv) || !commands_get_u8(arguments, length, 2, &bit) || !commands_get_u8(arguments, length, 3, &data))
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
    return;
  }

  if (!dcc_is_started())
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
    return;
  }

  dcc_mode_t mode = dcc_get_mode();
  // Address is zero based, cv IDs are 1 based
  uint16_t cvAddress = cv - 1;

  switch (mode)
  {
    case DCC_MODE_OPERATION:
    {
      // Use an operation mode command sequence
      output->writeln(COM_ERR);
      return;
    }
    case DCC_MODE_SERVICE:
    {
      // Use the service mode command sequence with acknowledgment
      if (!dcc_service_mode_set_cv_bit(cvAddress, bit, data, on_service_mode_result, (void*)output))
      {
        // Unable to start CV configuration process, return error
        output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
      }
      return;
    }
  }
}

static void verify_cv_bit_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  uint16_t address;
  uint16_t cv;
  uint8_t bit;
  uint8_t data;

  if (!commands_get_u16(arguments, length, 0, &address) || !commands_get_u16(arguments, length, 1, &cv) || !commands_get_u8(arguments, length, 2, &bit) || !commands_get_u8(arguments, length, 3, &data))
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_FORMAT));
    return;
  }

  if (!dcc_is_started())
  {
    output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
    return;
  }

  dcc_mode_t mode = dcc_get_mode();
  // Address is zero based, cv IDs are 1 based
  uint16_t cvAddress = cv - 1;

  switch (mode)
  {
    case DCC_MODE_OPERATION:
    {
      // Use an operation mode command sequence
      output->writeln(COM_ERR);
      return;
    }
    case DCC_MODE_SERVICE:
    {
      // Use the service mode command sequence with acknowledgment
      if (!dcc_service_mode_verify_cv_bit(cvAddress, bit, data, on_service_mode_result, (void*)output))
      {
        // Unable to start CV configuration process, return error
        output->writeln(ERR_WITH_REASON(COM_DCC_ERR_QUEUE));
      }
      return;
    }
  }
}

static void on_timer(uint8_t timer)
{
  arduino_toggle_led();
}