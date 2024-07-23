#ifndef DCC_H_
#define DCC_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  DCC_MESSAGE_FLAG_NONE = 0,
  DCC_MESSAGE_FLAG_USER_PROVIDED = 1,
  DCC_MESSAGE_FLAG_LONG_PREAMBLE = 2,
} dcc_message_flags_t;

typedef struct __attribute__((packed))
{
  uint8_t flags;
  uint8_t dccMessageId;
} dcc_event_message_t;

typedef enum
{
  DCC_MODE_OPERATION,
  DCC_MODE_SERVICE
} dcc_mode_t;

// Initializes the module
void dcc_initialize(void);

// Starts sending out signals
void dcc_start(dcc_mode_t mode);

// Stops sending out signals
void dcc_stop(void);

// Get the current state
bool dcc_is_started(void);

// Gets the current mode
dcc_mode_t dcc_get_mode(void);

// Call this when EVENT_FLAG_DCC_TX_STARTED is set
void dcc_on_tx_started(void);

// Call this when EVENT_FLAG_DCC_TX_COMPLETED is set
void dcc_on_tx_completed(void);

// Queues data for transmission
bool dcc_queue_data(const uint8_t* data, uint8_t size, uint8_t *messageIdOut);

// Queues data for transmission, does not set the USER_PROVIDED flag
bool dcc_queue_data_internal(const uint8_t* data, uint8_t size, uint8_t *messageIdOut);

#endif /* DCC_H_ */