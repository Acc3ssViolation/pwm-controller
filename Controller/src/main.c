#include "arduino/mega.h"
#include "arduino/gpio.h"
#include "arduino/button.h"
#include "arduino/serial.h"
#include "arduino/serial1.h"
#include "sysb/log.h"
#include "sysb/timer.h"
#include "sysb/buffers.h"
#include "sysb/commands.h"
#include "sysb/serial_console.h"
#include "dcc/dcc.h"
#include "dcc/dcc_commands.h"
#include "dcc/dcc_service_mode.h"
#include "util/delay.h"
#include "sysb/events.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/io.h>

static volatile uint16_t m_ticks;

int main(void)
{
  arduino_board_init();
  commands_initialize();
  log_initialize();
  timer_initialize();
  serial_console_initialize();
  dcc_commands_initialize();
  dcc_service_mode_initialize();
  
  // Set up timer 2 as a systick timer of 1 ms
  // No outputs, mode 2 (CTC) to clear on capture compare, we get an OC2A interrupt every time the counter gets to OCR2A
  TCCR2A = (1 << WGM21);
  TIMSK2 = (1 << OCIE2A);
  OCR2A = 250;
  TCCR2B = 4;   // Timer 2 uses different scaling values, so can't use the macro

  dcc_initialize();

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
    const uint16_t* adcData = (const uint16_t*)&message.data[0];
    const dcc_event_message_t* dccMessageData = (const dcc_event_message_t*)&message.data[0];

    if (event_get_message(&message))
    {
      switch (message.id)
      {
        case MESSAGE_ID_DCC_TX_STARTED:
        {
          dcc_on_tx_started();
          break;
        }
        case MESSAGE_ID_DCC_TX_COMPLETED:
        {
          dcc_on_tx_completed();
          dcc_service_mode_tx_complete(dccMessageData);

          if (dccMessageData->flags & DCC_MESSAGE_FLAG_USER_PROVIDED)
          {
            log_writeln_format(COM_DCC_TX_COMPLETE, dccMessageData->dccMessageId);
          }
          break;
        }
        case MESSAGE_ID_ADC_SAMPLES:
        {
          //static uint8_t ctr = 0;
          //if (++ctr == 0)
            //log_writeln_format("%u", *adcData);
          dcc_service_mode_on_current_sense_data(*adcData);
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