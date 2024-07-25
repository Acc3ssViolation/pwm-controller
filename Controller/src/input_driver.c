#include "platform.h"
#include "input_driver.h"
#include "gpio.h"
#include <avr/io.h>
#include <util/delay.h>

// These pins are using the internal pullup and are active low
const gpio_info_t PIN_FORWARDS = { .port = GPIO_PORT_C, .pin = GPIO_PIN_4 };
const gpio_info_t PIN_BACKWARDS = { .port = GPIO_PORT_C, .pin = GPIO_PIN_3 };

// This pin is also the ADC2 input
const gpio_info_t PIN_THROTTLE = { .port = GPIO_PORT_C, .pin = GPIO_PIN_2 };

void input_driver_initialize(void)
{
  gpio_set_pin(PIN_FORWARDS.port, PIN_FORWARDS.pin);
  gpio_configure_input(PIN_FORWARDS.port, PIN_FORWARDS.pin);
  gpio_set_pin(PIN_BACKWARDS.port, PIN_BACKWARDS.pin);
  gpio_configure_input(PIN_BACKWARDS.port, PIN_BACKWARDS.pin);
}

input_direction_t input_driver_get_direction(void)
{
  // TODO: This probably needs some debounce
  bool forwards = gpio_get_input(PIN_FORWARDS.port, PIN_FORWARDS.pin);
  bool backwards = gpio_get_input(PIN_BACKWARDS.port, PIN_BACKWARDS.pin);
  if (forwards && !backwards) {
    return INPUT_DIRECTION_FORWARDS;
  }
  else if (!forwards && backwards) {
    return INPUT_DIRECTION_BACKWARDS;
  }
  return INPUT_DIRECTION_IDLE;
}

uint16_t input_driver_get_throttle(void)
{
  // Vcc as ref, ADC2 as input
  ADMUX = BIT(REFS0) | ADC_CHANNEL_SINGLE_2;

  // Enable ADC, start a conversion
  ADCSRA = BIT(ADEN) | BIT(ADSC) | ADC_PRESCALER_16;

  // Wait for ADC to finish converting
  while ((ADCSRA & BIT(ADSC)) != 0) {
    _delay_us(1);
  }

  // Read the result and return it
  return ADCW;
}