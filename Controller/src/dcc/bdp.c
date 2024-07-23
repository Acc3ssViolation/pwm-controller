#include "dcc/bdp.h"
#include <string.h>
#include <util/crc16.h>

void bdp_port_initialize(bdp_port_t *port, bdp_transmit_t txFunction, bdp_handle_command_t handlerFunction)
{
  port->handlerFunction = handlerFunction;
  port->txFunction = txFunction;
  port->lastTransmittedResponseSize = 0;
  port->lastRxSeqNumber = 0;
}

bool bdp_port_received_command(bdp_port_t *port, const uint8_t *data, uint16_t dataLength)
{
  if (dataLength < sizeof(bdp_header_t))
  {
    // Does not even fit a header
    return false;
  }

  const bdp_header_t *header = (bdp_header_t*)data;
  uint16_t expectedLength = header->dataLength + sizeof(bdp_header_t) + BDP_CRC_LENGTH;

  if (dataLength < expectedLength)
  {
    // Missing part of the message
    return false;
  }

  // We have enough data, check the CRC
  uint16_t crc = BDP_CRC_INIT;
  for (uint16_t i = 0; i < expectedLength; i++)
  {
    crc = _crc_xmodem_update(crc, data[i]);
  }

  if (crc != BDP_CRC_CHECK)
  {
    // CRC error
    return false;
  }

  if (header->flags & BDP_FLAG_COMMAND)
  {
    bool isNewCommand = false;

    if (((header->flags & BDP_FLAG_RESET) == 0) && (port->lastRxSeqNumber == header->seqNumber))
    {
      // Repeated message and this is not a reset, we have already processed this!
      isNewCommand = false;
    }

    if (!isNewCommand && ((header->flags & BDP_FLAG_DO_NOT_RESPOND) == 0))
    {
      // Not a new command, re-transmit the result from the previous time
      if (port->lastTransmittedResponseSize > 0)
      {
        const bdp_header_t* responseHeader = (bdp_header_t*)&port->lastTransmittedResponse[0];
        if (responseHeader->seqNumber == header->seqNumber)
        {
          // Stored response is for this command, transmit it
          return port->txFunction(&port->lastTransmittedResponse[0], port->lastTransmittedResponseSize);
        }
      }

      // Did not have the previous result, error
      return false;
    }

    // Let the handler handle it
    if (!port->handlerFunction(header))
    {
      // Handler did not handle this command at all, return an unsupported response code
      uint8_t responseData = BDP_RESULT_UNKNOWN_COMMAND;

      (void) bdp_port_transmit_response(port, header->seqNumber, &responseData, sizeof(responseData));
    }

    port->lastRxSeqNumber = header->seqNumber;
    return true;
  }

  // A valid response, just pretend we handled it
  return true;
}

bool bdp_port_transmit_response(bdp_port_t *port, uint8_t seqNumber, const uint8_t *data, uint16_t dataLength)
{
  uint16_t totalSize = sizeof(bdp_header_t) + dataLength + BDP_CRC_LENGTH;

  if (totalSize > BDP_LAST_RESPONSE_SIZE)
  {
    // Message would not fit in response buffer
    return false;
  }

  // Create message
  bdp_header_t *header = (bdp_header_t*)&port->lastTransmittedResponse[0];
  header->flags = 0;
  header->seqNumber = seqNumber;
  header->dataLength = dataLength;

  memcpy(&port->lastTransmittedResponse[sizeof(bdp_header_t)], data, dataLength);

  // Calculate and append CRC
  uint16_t crcDataLength = dataLength + sizeof(bdp_header_t);
  uint16_t crc = BDP_CRC_INIT;
  for (uint16_t i = 0; i < crcDataLength; i++)
  {
    crc = _crc_xmodem_update(crc, data[i]);
  }

  port->lastTransmittedResponse[crcDataLength] = crc >> 8;
  port->lastTransmittedResponse[crcDataLength + 1] = crc & 0xFF;

  port->lastTransmittedResponseSize = totalSize;

  port->txFunction(&port->lastTransmittedResponse[0], port->lastTransmittedResponseSize);

  return false;
}
