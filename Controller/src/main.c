#include "platform.h"
#include "gpio.h"
#include "button.h"
#include "serial.h"
#include "log.h"
#include "timer.h"
#include "buffers.h"
#include "commands.h"
#include "serial_console.h"
#include "input_driver.h"
#include "pwm_driver.h"
#include "led_driver.h"

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

  input_driver_initialize();
  pwm_driver_initialize();
  led_driver_initialize();
  
  // Set up timer 2 as a systick timer of 1 ms
  // No outputs, mode 2 (CTC) to clear on capture compare, we get an OC2A interrupt every time the counter gets to OCR2A
  TCCR2A = (1 << WGM21);
  TIMSK2 = (1 << OCIE2A);
  OCR2A = 250;
  TCCR2B = 4;   // Timer 2 uses different scaling values, so can't use the macro

  // LED self test
  // led_driver_set(LED_ERROR, LED_MODE_ON);
  // _delay_ms(500);
  // led_driver_set(LED_PWM_ON, LED_MODE_ON);
  // _delay_ms(500);
  // led_driver_set(LED_PC_CONTROL, LED_MODE_ON);
  // _delay_ms(500);
  // led_driver_set(LED_ERROR, LED_MODE_DISABLED);
  // led_driver_set(LED_PWM_ON, LED_MODE_DISABLED);
  // led_driver_set(LED_PC_CONTROL, LED_MODE_DISABLED);

  log_writeln("PWM Controller V0");
  log_writeln("Type HELP for help");
  
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

    const input_direction_t in_dir = input_driver_get_direction();
    const uint16_t in_thr = input_driver_get_throttle();

    log_writeln_format("dir %d, thr %u", in_dir, in_thr);

    if (in_dir == INPUT_DIRECTION_FORWARDS)
    {
      pwm_driver_set_duty_cycle(in_thr / 4);
      pwm_driver_set_reversed(false);
      pwm_driver_set_enabled(true);

      led_driver_set(LED_PWM_ON, LED_MODE_BLINK);
    }
    else if (in_dir == INPUT_DIRECTION_BACKWARDS)
    {
      pwm_driver_set_duty_cycle(in_thr / 4);
      pwm_driver_set_reversed(true);
      pwm_driver_set_enabled(true);

      led_driver_set(LED_PWM_ON, LED_MODE_BLINK);
    }
    else
    {
      pwm_driver_set_duty_cycle(in_thr / 4);
      pwm_driver_set_enabled(false);

      led_driver_set(LED_PWM_ON, LED_MODE_DISABLED);
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
    _delay_ms(100);
  }
}

ISR(TIMER2_COMPA_vect)
{
  ++m_ticks;
  events_set_flags(EVENT_FLAG_TICK);
}