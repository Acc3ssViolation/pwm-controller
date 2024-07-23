#ifndef EVENTS_H_
#define EVENTS_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  EVENT_FLAG_NONE = 0,
  EVENT_FLAG_TICK,
} event_flags_t;

#define MAX_MESSAGE_PAYLOAD     (7)

typedef enum
{
  MESSAGE_ID_DCC_TX_STARTED,
  MESSAGE_ID_DCC_TX_COMPLETED,
  MESSAGE_ID_ADC_SAMPLES,
} message_id_t;

typedef struct 
{
  message_id_t id;
  uint8_t data[MAX_MESSAGE_PAYLOAD]; 
} message_t;

void events_set_flags(event_flags_t flags);

bool events_read_and_clear_flags(event_flags_t flags);

event_flags_t events_get_and_clear_flags(void);

bool event_post_message(const message_t *message);

bool event_get_message(message_t *message);


#endif /* EVENTS_H_ */