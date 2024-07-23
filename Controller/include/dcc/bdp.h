#ifndef BDP_H_
#define BDP_H_

#include <stdint.h>
#include <stdbool.h>

#define BDP_CRC_LENGTH      (2)       // We use CRC-16-XMODEM, so 2 bytes

#define BDP_CRC_INIT    (0x0000)
#define BDP_CRC_CHECK   (0x0000)

#define BDP_LAST_RESPONSE_SIZE    (64)

typedef enum
{
  BDP_FLAG_NONE = 0,
  BDP_FLAG_COMMAND = 1,
  BDP_FLAG_DO_NOT_RESPOND = 2,
  BDP_FLAG_RESET = 4,
} bdp_flags_t;

typedef struct __attribute__((packed))
{
  uint8_t flags;
  uint8_t seqNumber;
  uint8_t dataLength;
} bdp_header_t;

typedef bool (*bdp_transmit_t)(const uint8_t *data, uint16_t size);

typedef bool (*bdp_handle_command_t)(const bdp_header_t *header);

typedef struct  
{
  uint8_t lastRxSeqNumber;
  bdp_transmit_t txFunction; 
  bdp_handle_command_t handlerFunction;
  uint8_t lastTransmittedResponseSize;
  uint8_t lastTransmittedResponse[BDP_LAST_RESPONSE_SIZE];
} bdp_port_t;

typedef enum
{
  BDP_RESULT_OK = 0,
  BDP_RESULT_UNKNOWN_COMMAND = 1,
} bdp_result_code_t;

void bdp_port_initialize(bdp_port_t *port, bdp_transmit_t txFunction, bdp_handle_command_t handlerFunction);

bool bdp_port_received_command(bdp_port_t *port, const uint8_t *data, uint16_t dataLength);

bool bdp_port_transmit_response(bdp_port_t *port, uint8_t seqNumber, const uint8_t *data, uint16_t dataLength);



#endif /* BDP_H_ */