#include "platform.h"
#include "gpio.h"
#include "button.h"
#include "serial.h"
#include "log.h"
#include "timer.h"
#include "buffers.h"
#include "commands.h"
#include "serial_console.h"
#include "util/delay.h"
#include "events.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/io.h>

static volatile uint16_t m_ticks;

int main(void)
{
  commands_initialize();
  log_initialize();
  timer_initialize();
  serial_console_initialize();
  
  // Set up timer 2 as a systick timer of 1 ms
  // No outputs, mode 2 (CTC) to clear on capture compare, we get an OC2A interrupt every time the counter gets to OCR2A
  TCCR2A = (1 << WGM21);
  TIMSK2 = (1 << OCIE2A);
  OCR2A = 250;
  TCCR2B = 4;   // Timer 2 uses different scaling values, so can't use the macro

  uint16_t previousTicks = m_ticks;
  sei();



  while (1)
  {
    event_flags_t flags = events_get_and_clear_flags();
    if (flags & EVENT_FLAG_TICK)
    {
      uint16_t currentTicks = m_ticks;
      timer_tick(currentTicks - previousTicks);
      previousTicks = currentTicks;
    }

    message_t message;
    if (event_get_message(&message))
    {
      switch (message.id)
      {
        case MESSAGE_ID_DCC_TX_STARTED:
        {
          break;
        }
        case MESSAGE_ID_DCC_TX_COMPLETED:
        {
          break;
        }
        case MESSAGE_ID_ADC_SAMPLES:
        {
          const uint16_t* adcData = (const uint16_t*)&message.data[0];
          static uint8_t ctr = 0;
          if (++ctr == 0)
            log_writeln_format("%u", *adcData);
          break;
        }
      }
    }
    
    serial_console_poll();
  }
}

ISR(TIMER2_COMPA_vect)
{
  ++m_ticks;
  events_set_flags(EVENT_FLAG_TICK);
}