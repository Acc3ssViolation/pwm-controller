#include "commands.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_COMMANDS    (32)

static uint8_t m_commandCount;
static const command_t* m_commands[MAX_COMMANDS];

static void help_command(const char *arguments, uint8_t length, const command_functions_t* output);

static const char* find_next_argument(const char* input, uint8_t inputLength);

static const command_t m_helpCommand = {
  .prefix = "HELP",
  .summary = "Provides info about commands",
  .handler = help_command
};

void commands_initialize(void)
{
  m_commandCount = 0;

  commands_register(&m_helpCommand);
}

void commands_register(const command_t* command)
{
  if (m_commandCount < MAX_COMMANDS)
  {
    m_commands[m_commandCount++] = command;
  }
}

void commands_handle(const char* input, uint8_t inputLength, const command_functions_t* output)
{
  for (uint8_t i = 0; i < m_commandCount; i++)
  {
    const command_t* command = m_commands[i];
    uint8_t prefixLength = strlen(command->prefix);
    uint8_t inputPrefixLength;
    for(inputPrefixLength = 0; inputPrefixLength < inputLength; inputPrefixLength++)
    {
      if (input[inputPrefixLength] == ' ')
        break;
    }

    if (prefixLength != inputPrefixLength)
    {
      continue;
    }
    
    if (strncasecmp(input, command->prefix, prefixLength) != 0)
    {
      continue;
    }

    while ((inputPrefixLength < inputLength) && (input[inputPrefixLength] == ' '))
    {
      inputPrefixLength++;
    }

    command->handler(&input[inputPrefixLength], inputLength - inputPrefixLength, output);
    return;
  }

  output->writeln(ERR_WITH_REASON(COM_ERR_UNKNOWN));
}

bool commands_match(const char* input, uint8_t inputLength, const char* value)
{
  uint8_t valueLength = strlen(value);
  if (inputLength < valueLength)
  {
    return false;
  }
  return (strncasecmp(input, value, inputLength) == 0);
}

bool commands_get_u8(const char* input, uint8_t inputLength, uint8_t argumentIndex, uint8_t* result)
{
  uint16_t temp;
  if (commands_get_u16(input, inputLength, argumentIndex, &temp))
  {
    if (temp <= UINT8_MAX)
    {
      *result = temp;
      return true;
    }
  }

  return false;
}

bool commands_get_u16(const char* input, uint8_t inputLength, uint8_t argumentIndex, uint16_t* result)
{
  // Find start of argument
  const char* argumentStart = input;
  for (;argumentIndex > 0; argumentIndex--)
  {
    const char* nextArgument = find_next_argument(argumentStart, inputLength);
    if (nextArgument == NULL)
    {
      return false;
    }

    inputLength -= (nextArgument - argumentStart);
    argumentStart = nextArgument;
  }

  // Parse the argument
  errno = 0;
  char* end;
  uint32_t parsedResult = strtoul(argumentStart, &end, 0);
  if ((parsedResult > UINT16_MAX) || (errno != 0) || (argumentStart == end))
  {
    return false;
  }
  *result = parsedResult;
  return true;
}

bool commands_get_string(const char* input, uint8_t inputLength, uint8_t argumentIndex, const char** result, uint8_t *resultLength)
{
  // Find start of argument
  const char* argumentStart = input;
  for (;argumentIndex > 0; argumentIndex--)
  {
    const char* nextArgument = find_next_argument(argumentStart, inputLength);
    if (nextArgument == NULL)
    {
      return false;
    }

    inputLength -= (nextArgument - argumentStart);
    argumentStart = nextArgument;
  }

  // Parse the argument
  *result = argumentStart;
  for (;inputLength > 0; inputLength--, argumentStart++)
  {
    if (*argumentStart == ' ')
    { 
      break;
    }
  }
  *resultLength = (argumentStart - *result);
  return true;
}

bool commands_get_on_off(const char* input, uint8_t inputLength, uint8_t argumentIndex, bool* result)
{
  const char *arg = NULL;
  uint8_t argLength = 0;
  if (false == commands_get_string(input, inputLength, argumentIndex, &arg, &argLength))
  {
    return false;
  }

  if (argLength == 2 && strncasecmp(arg, "on", argLength) == 0)
  {
    *result = true;
    return true;
  }
  else if (argLength == 3 && strncasecmp(arg, "off", argLength) == 0)
  {
    *result = false;
    return true;
  }
  else
  {
    return false;
  }
}

static const char* find_next_argument(const char* input, uint8_t inputLength)
{
  // Find next space
  for (;inputLength > 0; inputLength--, input++)
  {
    if (*input == ' ')
    {
      break;
    }
  }

  if (inputLength == 0)
  {
    // Ran out of input without finding a space
    return NULL;
  }

  // Find the first non-space character
  for (;inputLength > 0; inputLength--, input++)
  {
    if (*input != ' ')
    {
      return input;
    }
  }

  // Ran out of input without finding something behind the spaces
  return NULL;
}

static void help_command(const char *arguments, uint8_t length, const command_functions_t* output)
{
  for (int i = 0; i < m_commandCount; i++)
  {
    const command_t* command = m_commands[i];
    output->writeln(command->prefix);
  }
}