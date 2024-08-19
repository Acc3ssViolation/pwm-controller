#include "platform.h"
#include "pwm_driver.h"
#include "gpio.h"
#include "atomic.h"
#include <avr/interrupt.h>
#include <avr/io.h>

const uint16_t PWM_TOP = 0x2FF;
const gpio_info_t PIN_DIR = {.port = GPIO_PORT_B, .pin = GPIO_PIN_2};       // OC1B
const gpio_info_t PIN_BRAKE = {.port = GPIO_PORT_D, .pin = GPIO_PIN_5};     // OC0B
const gpio_info_t PIN_PWM = {.port = GPIO_PORT_D, .pin = GPIO_PIN_6};       // OC0A
const gpio_info_t PIN_NTHERMAL = {.port = GPIO_PORT_D, .pin = GPIO_PIN_4};  // PCINT20

static bool _thermalError = false;

static void shutdown(void);

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

  // Set up the timer to control PIN_PWM (OC0A)
  // We need to use Phase Correct PWM mode, no prescaler.
  // According to the docs we get a PWM frequency of (Fclk) / (N * 510)
  // which for our usecase gives 31372... Hz, which is adequate.
  OCR0A = 0;
  TCCR0A = BIT(WGM00) | BIT(COM0A1);
  TCCR0B = BIT(CS00);

  // Set up thermal interrupt
  PCICR |= BIT(PCIE2);
  PCMSK2 |= BIT(PCINT20);
}

void pwm_driver_set_enabled(bool enabled)
{
  if (enabled)
  {
    gpio_reset_pin(PIN_BRAKE.port, PIN_BRAKE.pin);
  }
  else
  {
    OCR0A = 0;
    gpio_set_pin(PIN_BRAKE.port, PIN_BRAKE.pin);
  }

  if (_thermalError)
  {
    shutdown();
  }
}

void pwm_driver_set_duty_cycle(uint8_t dutyCycle)
{
  OCR0A = dutyCycle;

  if (_thermalError)
  {
    shutdown();
  }
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

  if (_thermalError)
  {
    shutdown();
  }
}

bool pwm_driver_is_error(void)
{
  return _thermalError;
}

ISR(PCINT2_vect)
{
  if (false == gpio_get_input(PIN_NTHERMAL.port, PIN_NTHERMAL.pin))
  {
    if (false == _thermalError)
    {
      shutdown();
    }
    _thermalError = true;
  }
}

static void shutdown(void)
{
  // Disable PWM timer output
  TCCR0A = 0;
  TCCR0B = 0;

  // Disconnect outputs
  gpio_set_pin(PIN_BRAKE.port, PIN_BRAKE.pin);
  gpio_reset_pin(PIN_PWM.port, PIN_PWM.pin);
}