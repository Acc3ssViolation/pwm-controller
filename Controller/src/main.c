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
#include "locomotive_settings.h"

#include "util/delay.h"
#include "events.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#define CONTROL_INTERVAL_MS 100 // 10 Hz control loop

const uint16_t PC_TIMEOUT_MS = 4000;

static uint16_t m_ticks;

static bool m_flipped = false;
static bool m_pcControl = false;
static int16_t m_pcSpeed = 0;
static uint8_t m_pcTimeoutTimer;
static bool m_debug = false;

static void control_task(uint8_t timerHandle);

static void pc_timer_callback(uint8_t timerHandle);

static void pc_command(const char *arguments, uint8_t length, const command_functions_t *output);

static void dc_command(const char *arguments, uint8_t length, const command_functions_t *output);

static void reset_command(const char *arguments, uint8_t length, const command_functions_t *output);

static void debug_command(const char *arguments, uint8_t length, const command_functions_t *output);

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

static const command_t m_debugCommand = {
    .prefix = "DEBUG",
    .summary = "Debug live values",
    .handler = debug_command,
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

  locomotive_settings_initialize();

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
  commands_register(&m_debugCommand);
  commands_register(&m_resetCommand);

  m_pcTimeoutTimer = timer_create(TIMER_MODE_SINGLE, pc_timer_callback);

  uint8_t controlTimer = timer_create(TIMER_MODE_REPEATING, control_task);
  timer_start(controlTimer, CONTROL_INTERVAL_MS);

  log_writeln("================================");
  log_writeln("||     PWM Controller V0      ||");
  log_writeln("||        "__DATE__ "         ||");
  log_writeln("||    Type HELP for help!     ||");
  log_writeln("================================");

  uint16_t previousTicks = m_ticks;
  sei();

  while (1)
  {
    event_flags_t flags = events_get_and_clear_flags();
    if (flags & EVENT_FLAG_TICK)
    {
      cli();
      uint16_t currentTicks = m_ticks;
      sei();
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
        const uint16_t *adcData = (const uint16_t *)&message.data[0];
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

    m_pcSpeed = arg1;
    output->writeln(COM_OK);

    timer_start(m_pcTimeoutTimer, PC_TIMEOUT_MS);
  }
  else if (commands_match(arg0, arg0Length, "REV"))
  {
    if (false == commands_get_u8(arguments, length, 1, &arg1))
    {
      output->writeln(ERR_WITH_REASON("Missing speed argument"));
      return;
    }

    m_pcSpeed = arg1;
    m_pcSpeed = -m_pcSpeed;
    output->writeln(COM_OK);

    timer_start(m_pcTimeoutTimer, PC_TIMEOUT_MS);
  }
  else if (commands_match(arg0, arg0Length, "STOP"))
  {
    m_pcSpeed = 0;
    output->writeln(COM_OK);

    timer_stop(m_pcTimeoutTimer);
  }
  else if (commands_match(arg0, arg0Length, "FLIP"))
  {
    bool flipped = false;
    if (commands_get_on_off(arguments, length, 1, &flipped))
    {
      m_flipped = flipped;
    }
    else
    {
      output->writeln(ERR_WITH_REASON("Missing on/off argument"));
      return;
    }
  }
  else
  {
    output->writeln(ERR_WITH_REASON("Missing fwd/rev/stop argument"));
  }
}

static void reset_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  output->writeln(COM_ERR);
}

static void debug_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  bool enabled = false;
  if (false == commands_get_on_off(arguments, length, 0, &enabled))
  {
    output->writeln_format(ERR_WITH_REASON("Use on/off to enable/disable debug logging"));
    return;
  }

  m_debug = enabled;
}

static void pc_timer_callback(uint8_t timerHandle)
{
  (void)timerHandle;
  m_pcSpeed = 0;
}

static void control_task(uint8_t timerHandle)
{
  static uint8_t m_boostTimeLeft = 0;
  static uint8_t m_activeSpeedStep = 0;
  static bool m_activeReversed = false;

  // TODO: The first time through this task these readings appear to be wrong
  input_direction_t inDir = input_driver_get_direction();
  int16_t inThrottle = input_driver_get_throttle();
  if (inDir == INPUT_DIRECTION_BACKWARDS)
  {
    inThrottle = -inThrottle;
  }
  else if (inDir == INPUT_DIRECTION_IDLE)
  {
    inThrottle = 0;
  }
  bool thermal_err = pwm_driver_is_error();

  if (m_pcControl)
  {
    inThrottle = m_pcSpeed * 4;
  }

  // Map throttle to desired speed step
  // TODO: Non-linear mapping?
  uint8_t desiredSpeedStep = abs(inThrottle / 4);
  bool desiredReversed = inThrottle < 0;

  const locomotive_profile_t *profile = locomotive_settings_get_active();

  // If we are moving and a change in direction is requested we must first try to stop
  if (m_activeSpeedStep != 0 && desiredReversed != m_activeReversed)
  {
    desiredSpeedStep = 0;
  }

  // Direction flips can only happen when we are stationary
  if (m_activeSpeedStep == 0 && desiredReversed != m_activeReversed)
  {
    m_activeReversed = desiredReversed;
  }

  uint8_t previousSpeedStep = m_activeSpeedStep;
  m_activeSpeedStep = locomotive_settings_apply_speed(profile, m_activeSpeedStep, desiredSpeedStep, CONTROL_INTERVAL_MS);
  uint8_t applied_voltage = locomotive_settings_map_speed(profile, m_activeSpeedStep);
  
  if (profile->boostPower != 0)
  {
    // Start the boost timer when we move from step 0 to step 1
    if (previousSpeedStep == 0 && m_activeSpeedStep != 0)
    {
      if (m_boostTimeLeft == 0)
      {
        log_writeln("Started boost");
        m_boostTimeLeft = 100;
      }
    }

    // Apply boost while timer is active
    if (m_boostTimeLeft > 0)
    {
      if (m_boostTimeLeft > CONTROL_INTERVAL_MS)
      {
        m_boostTimeLeft -= CONTROL_INTERVAL_MS;
      }
      else
      {
        m_boostTimeLeft = 0;
      }

      if (m_boostTimeLeft == 0)
      {
        log_writeln("Stopped boost");
      }

      // Guard to make sure we don't accidentally anti-boost when we have already moved up to higher speed steps
      if (profile->boostPower > applied_voltage)
      {
        applied_voltage = profile->boostPower;
      }
    }
  }
  else if (m_boostTimeLeft != 0)
  {
    log_writeln("Stopped boost");
    m_boostTimeLeft = 0;
  }

  bool reversed = m_activeReversed;
  bool disabled = !m_pcControl && inDir == INPUT_DIRECTION_IDLE && m_activeSpeedStep == 0;

  // Apply forward trim if configured
  if (!reversed && profile->fwdTrim != 0)
  {
    uint16_t tmp = applied_voltage;
    tmp = tmp * (uint16_t)profile->fwdTrim;
    tmp /= 256;
    applied_voltage = tmp;
  }

  // Track voltage needs to be flipped if locomotive is flipped
  reversed ^= m_flipped;

  if (m_debug)
  {
    log_writeln_format("as: %u, ar: %u, ds: %u, dr: %u, v:%u", m_activeSpeedStep, m_activeReversed, desiredSpeedStep, desiredReversed, applied_voltage);
  }

  if (thermal_err)
  {
    pwm_driver_set_duty_cycle(0);
    pwm_driver_set_enabled(false);
    led_driver_set(LED_ERROR, LED_MODE_ON);
    led_driver_set(LED_PWM_ON, LED_MODE_DISABLED);
  }
  else if (disabled)
  {
    pwm_driver_set_duty_cycle(0);
    pwm_driver_set_enabled(false);
    led_driver_set(LED_PWM_ON, LED_MODE_DISABLED);
  }
  else
  {
    pwm_driver_set_duty_cycle(applied_voltage);
    pwm_driver_set_reversed(reversed);
    pwm_driver_set_enabled(true);

    led_driver_set(LED_PWM_ON, LED_MODE_BLINK);
  }
}