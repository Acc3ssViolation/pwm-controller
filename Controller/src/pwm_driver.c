#include "platform.h"
#include "pwm_driver.h"
#include "gpio.h"
#include <avr/io.h>

const uint16_t PWM_TOP = 0x2FF;
const gpio_info_t PIN_DIR = {.port = GPIO_PORT_B, .pin = GPIO_PIN_2};       // OC1B
const gpio_info_t PIN_BRAKE = {.port = GPIO_PORT_D, .pin = GPIO_PIN_5};     // OC0B
const gpio_info_t PIN_PWM = {.port = GPIO_PORT_D, .pin = GPIO_PIN_6};       // OC0A
const gpio_info_t PIN_NTHERMAL = {.port = GPIO_PORT_D, .pin = GPIO_PIN_4};  // PCINT20

// LMD18200 truth table
// From TI datasheet page 8
// PWM  DIR  BRK  OUT
// 1    1    0    Source 1, Sink 2        Out 1 V+
// 1    0    0    Sink 1, Source 2        Out 2 V+
// 0    X    0    Source 1, Source 2      Both tied to V+
// 1    1    1    Source 1, Source 2      Both tied to V+
// 1    0    1    Sink 1, Sink 2          Both tied to V-
// 0    X    1    None                    Outputs disconnected
//
// From this table we can conclude the following operating modes:
// For DCC we want to PWM on the DIR pin, which will give complementary outputs on Out 1 and Out 2  (PWM = 1, BRK = 0)
// For analog PWM we can PWM on the BRK pin, which will PWM one of the outputs while keeping the other grounded (PWM = 1, DIR = DIR)
// For analog PWM we can also PWM on the PWM pin instead, leaving BRK set to 0. This means both outputs are set to V+ instead of V- when "disabled"
//
// The minimum time between changing different pins should be 1 microsecond.

void pwm_driver_initialize(void)
{
  // Disable outputs on startup
  gpio_set_pin(PIN_BRAKE.port, PIN_BRAKE.pin);
  gpio_configure_output(PIN_BRAKE.port, PIN_BRAKE.pin);
  gpio_reset_pin(PIN_PWM.port, PIN_PWM.pin);
  gpio_configure_output(PIN_PWM.port, PIN_PWM.pin);
  gpio_reset_pin(PIN_DIR.port, PIN_DIR.pin);
  gpio_configure_output(PIN_DIR.port, PIN_DIR.pin);

  // Pull up
  gpio_set_pin(PIN_NTHERMAL.port, PIN_NTHERMAL.pin);

  // // Set up the timer
  // // We use Fast PWM mode using ICR1 as the max value. We don't use a prescaler
  // // so we end up with F_CPU / 0x2FF which ends up at 20860.49... Hz for a 16 MHz
  // // clock frequency. This gives us about 9.5 bits worth of resolution.
  // ICR1 = PWM_TOP;

  // gpio_reset_pin(PIN_ENABLED.port, PIN_ENABLED.pin);
  // TCCR1B = BIT(WGM13) | BIT(WGM12) | BIT(CS10);
  // TCCR1A = BIT(WGM11);
}

void pwm_driver_set_enabled(bool enabled)
{
  if (enabled)
  {
    gpio_set_pin(PIN_PWM.port, PIN_PWM.pin);
    gpio_reset_pin(PIN_BRAKE.port, PIN_BRAKE.pin);
  }
  else
  {
    gpio_reset_pin(PIN_PWM.port, PIN_PWM.pin);
    gpio_set_pin(PIN_BRAKE.port, PIN_BRAKE.pin);
  }
}

void pwm_driver_set_duty_cycle(uint8_t dutyCycle)
{
}

void pwm_driver_set_reversed(bool reversed)
{
  if (reversed)
  {
    gpio_reset_pin(PIN_DIR.port, PIN_DIR.pin);
  }
  else
  {
    gpio_set_pin(PIN_DIR.port, PIN_DIR.pin);
  }
}