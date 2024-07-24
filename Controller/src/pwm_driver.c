#include "platform.h"
#include "pwm_driver.h"
#include "gpio.h"
#include <avr/io.h>

const uint16_t PWM_TOP = 0x2FF;
const gpio_info_t PIN_ENABLED = {.port = GPIO_PORT_B, .pin = GPIO_PIN_5};

static bool m_reversed;
static uint8_t m_dutyCycle;

void pwm_driver_initialize(void)
{
  gpio_reset_pin(PIN_ENABLED.port, PIN_ENABLED.pin);
  gpio_configure_output(PIN_ENABLED.port, PIN_ENABLED.pin);

  // OC1A (PB1) and OC1B (PB2) are used as output
  gpio_reset_pin(GPIO_PORT_B, GPIO_PIN_1);
  gpio_reset_pin(GPIO_PORT_B, GPIO_PIN_2);
  gpio_configure_output(GPIO_PORT_B, GPIO_PIN_1);
  gpio_configure_output(GPIO_PORT_B, GPIO_PIN_2);

  // Set up the timer
  // We use Fast PWM mode using ICR1 as the max value. We don't use a prescaler
  // so we end up with F_CPU / 0x2FF which ends up at 20860.49... Hz for a 16 MHz
  // clock frequency. This gives us about 9.5 bits worth of resolution.
  ICR1 = PWM_TOP;
}

void pwm_driver_set_enabled(bool enabled)
{
  if (enabled)
  {
    gpio_set_pin(PIN_ENABLED.port, PIN_ENABLED.pin);
    TCCR1B = BIT(WGM13) | BIT(WGM12) | BIT(CS10);
  }
  else
  {
    gpio_reset_pin(PIN_ENABLED.port, PIN_ENABLED.pin);
    TCCR1B = BIT(WGM13) | BIT(WGM12) | BIT(CS10);
  }
}

void pwm_driver_set_duty_cycle(uint8_t dutyCycle)
{
  if (m_dutyCycle == dutyCycle)
  {
    return;
  }

  // Map the value to our full range
  uint32_t tmp = dutyCycle;
  tmp *= 0x2FFUL;
  tmp /= 0xFF;

  OCR1A = (uint16_t)tmp;
  OCR1B = (uint16_t)tmp;

  if (dutyCycle == 0)
  {
    pwm_driver_set_enabled(false);
  }
  else
  {
    pwm_driver_set_enabled(true);
  }

  m_dutyCycle = dutyCycle;
}

void pwm_driver_set_reversed(bool reversed)
{
  if (reversed == m_reversed)
  {
    return;
  }

  if (reversed) 
  {
    // Channel B is PWM output, channel A is regular output set to ground
    TCCR1A = BIT(COM1B1) | BIT(WGM11);
  }
  else 
  {
    // Channel A is PWM output, channel B is regular output set to ground
    TCCR1A = BIT(COM1A1) | BIT(WGM11);
  }

  m_reversed = reversed;
}