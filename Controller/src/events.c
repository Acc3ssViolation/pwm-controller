#include "events.h"
#include <avr/interrupt.h>
#include <string.h>

#define MAX_MESSAGE_COUNT     (4)       // Maximum amount of messages we can store. MUST BE A POWER OF TWO.

static volatile event_flags_t flags;
static volatile uint8_t messageCount;
static volatile uint8_t messageReadIndex;
static volatile message_t messages[MAX_MESSAGE_COUNT];

void events_set_flags(event_flags_t flagsToSet)
{
  flags |= flagsToSet;
}

bool events_read_and_clear_flags(event_flags_t flagsToRead)
{
  bool result = false;

  cli();
  {
    if ((flags & flagsToRead) == flagsToRead)
    {
      flags &= ~flagsToRead;
      result = true;
      return true;
    }
  }
  sei();
  
  return result;
}

event_flags_t events_get_and_clear_flags(void)
{
  if (flags == 0)
  {
    return 0;
  }

  event_flags_t result;
  cli();
  {
    result = flags;
    flags = 0;
  }
  sei();
  return result;
}

bool event_post_message(const message_t *message)
{
  if (messageCount + 1 > MAX_MESSAGE_COUNT)
  {
    return false;
  }

  // Find write pointer
  uint8_t writeIndex =(messageReadIndex + messageCount) & (MAX_MESSAGE_COUNT - 1);;

  memcpy((void*)&messages[writeIndex], message, sizeof(message_t));
  messageCount++;

  return true;
}

bool event_get_message(message_t *message)
{
  if (messageCount < 1)
  {
    return false;
  }

  cli();
  {
    memcpy(message, (void*)&messages[messageReadIndex], sizeof(message_t));
    messageReadIndex = (messageReadIndex + 1) & (MAX_MESSAGE_COUNT - 1);
    messageCount--;
  }
  sei();

  return true;
}