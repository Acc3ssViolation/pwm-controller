#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <stdbool.h>
#include <stdint.h>

#define COM_DCC_ERR_FORMAT      "INVALID FORMAT"
#define COM_DCC_ERR_SIZE        "INVALID LENGTH"
#define COM_DCC_ERR_QUEUE       "QUEUE FULL"
#define COM_DCC_TX_COMPLETE     ":TXC ID %u"

#define COM_CRLF             "\r\n"
#define COM_OK              "OK"
#define COM_ERR             "ERR"
#define COM_ERR_UNKNOWN     "UNKNOWN COMMAND"

#define ERR_WITH_REASON(reason) (COM_ERR "+" COM_CRLF reason)
#define OK_WITH_RESULT(result)  (COM_OK "+" COM_CRLF result)

typedef void (*command_writeln_t)(const char *message);
typedef void (*command_writeln_format_t)(const char *format, ...);
typedef void (*command_write_t)(const char *message);
typedef void (*command_write_format_t)(const char *format, ...);

typedef struct
{
  command_write_t write;
  command_write_format_t write_format;
  command_writeln_t writeln;
  command_writeln_format_t writeln_format;
} command_functions_t;

typedef void (*command_handler_t)(const char *arguments, uint8_t length, const command_functions_t* output);

typedef struct  
{
  const char* prefix;
  const char* summary;
  command_handler_t handler;
} command_t;

void commands_initialize(void);

void commands_register(const command_t* command);

void commands_handle(const char* input, uint8_t inputLength, const command_functions_t* output);

bool commands_match(const char* input, uint8_t inputLength, const char* value);

bool commands_get_u8(const char* input, uint8_t inputLength, uint8_t argumentIndex, uint8_t* result);

bool commands_get_u16(const char* input, uint8_t inputLength, uint8_t argumentIndex, uint16_t* result);

bool commands_get_string(const char* input, uint8_t inputLength, uint8_t argumentIndex, const char** result, uint8_t *resultLength);

#endif /* COMMANDS_H_ */