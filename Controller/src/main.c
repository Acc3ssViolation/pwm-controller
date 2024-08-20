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
#include <avr/wdt.h>

static volatile uint16_t m_ticks;

static bool m_pcControl = false;
static uint8_t m_pcSpeed = 0;
static input_direction_t m_pcDirection = INPUT_DIRECTION_IDLE;

static void pc_command(const char *arguments, uint8_t length, const command_functions_t *output);

static void dc_command(const char *arguments, uint8_t length, const command_functions_t *output);

static void reset_command(const char *arguments, uint8_t length, const command_functions_t *output);

static const command_t m_pcCommand = {
    .prefix = "PC",
    .summary = "Enables or disables PC control",
    .handler = pc_command,
};

static const command_t m_dcCommand = {
    .prefix = "DC",
    .summary = "DC mode command",
    .handler = dc_command,
};

static const command_t m_resetCommand = {
    .prefix = "RESET",
    .summary = "Reset the device",
    .handler = reset_command,
};

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
  TCCR2B = 4; // Timer 2 uses different scaling values, so can't use the macro

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

  commands_register(&m_pcCommand);
  commands_register(&m_dcCommand);
  commands_register(&m_resetCommand);

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

    input_direction_t in_dir = input_driver_get_direction();
    uint16_t in_thr = input_driver_get_throttle();
    static uint16_t smooth_thr = 0;
    bool thermal_err = pwm_driver_is_error();

    if (m_pcControl)
    {
      in_dir = m_pcDirection;
      in_thr = m_pcSpeed * 4;
      smooth_thr = ((in_thr) + (9ul * smooth_thr)) / 10ul;
    }
    else
    {
      // Exponential filter on throttle input :)
      smooth_thr = ((in_thr) + (9ul * smooth_thr)) / 10ul;
      // log_writeln_format("dir %d, thr %u, sthr %u", in_dir, in_thr, smooth_thr);
    }

    if (thermal_err)
    {
      smooth_thr = in_thr;
      pwm_driver_set_enabled(false);
      led_driver_set(LED_ERROR, LED_MODE_ON);
      led_driver_set(LED_PWM_ON, LED_MODE_DISABLED);
    }
    else if (in_dir == INPUT_DIRECTION_FORWARDS)
    {
      pwm_driver_set_duty_cycle(smooth_thr / 4);
      pwm_driver_set_reversed(false);
      pwm_driver_set_enabled(true);

      led_driver_set(LED_PWM_ON, LED_MODE_BLINK);
    }
    else if (in_dir == INPUT_DIRECTION_BACKWARDS)
    {
      pwm_driver_set_duty_cycle(smooth_thr / 4);
      pwm_driver_set_reversed(true);
      pwm_driver_set_enabled(true);

      led_driver_set(LED_PWM_ON, LED_MODE_BLINK);
    }
    else
    {
      pwm_driver_set_duty_cycle(in_thr / 4);
      pwm_driver_set_enabled(false);
      smooth_thr = in_thr;

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
        const uint16_t *adcData = (const uint16_t *)&message.data[0];
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

static void pc_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  const char *arg0 = NULL;
  uint8_t arg0Length = 0;
  if (false == commands_get_string(arguments, length, 0, &arg0, &arg0Length))
  {
    output->writeln(ERR_WITH_REASON("Missing on/off argument"));
    return;
  }

  if (commands_match(arg0, arg0Length, "ON"))
  {
    m_pcControl = true;
    led_driver_set(LED_PC_CONTROL, LED_MODE_ON);
    output->writeln(COM_OK);
  }
  else if (commands_match(arg0, arg0Length, "OFF"))
  {
    m_pcControl = false;
    led_driver_set(LED_PC_CONTROL, LED_MODE_DISABLED);
    output->writeln(COM_OK);
  }
  else
  {
    output->writeln(ERR_WITH_REASON("Missing on/off argument"));
  }
}

static void dc_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  const char *arg0 = NULL;
  uint8_t arg0Length = 0;
  if (false == commands_get_string(arguments, length, 0, &arg0, &arg0Length))
  {
    output->writeln(ERR_WITH_REASON("Missing fwd/rev/stop argument"));
    return;
  }

  uint8_t arg1 = 0;

  if (commands_match(arg0, arg0Length, "FWD"))
  {
    if (false == commands_get_u8(arguments, length, 1, &arg1))
    {
      output->writeln(ERR_WITH_REASON("Missing speed argument"));
      return;
    }

    m_pcDirection = INPUT_DIRECTION_FORWARDS;
    m_pcSpeed = arg1;
    output->writeln(COM_OK);
  }
  else if (commands_match(arg0, arg0Length, "REV"))
  {
    if (false == commands_get_u8(arguments, length, 1, &arg1))
    {
      output->writeln(ERR_WITH_REASON("Missing speed argument"));
      return;
    }

    m_pcDirection = INPUT_DIRECTION_BACKWARDS;
    m_pcSpeed = arg1;
    output->writeln(COM_OK);
  }
  else if (commands_match(arg0, arg0Length, "STOP"))
  {
    m_pcDirection = INPUT_DIRECTION_IDLE;
    m_pcSpeed = 0;
    output->writeln(COM_OK);
  }
  else
  {
    output->writeln(ERR_WITH_REASON("Missing fwd/rev/stop argument"));
  }
}

static void reset_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  output->writeln(COM_OK);

  wdt_enable(WDTO_15MS);
  while (1)
  {
  }
}