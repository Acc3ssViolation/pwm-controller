#include "button.h"

void button_initialize(button_t* button)
{
  gpio_configure_input(button->gpio.port, button->gpio.pin);
  button->previousState = gpio_get_input(button->gpio.port, button->gpio.pin);
}

button_state_t button_update(button_t* button)
{
  bool newState = gpio_get_input(button->gpio.port, button->gpio.pin);
  button_state_t result;

  if (newState == button->previousState)
  {
    result = newState ? BUTTON_STAYED_DOWN : BUTTON_STAYED_UP;
  }
  else
  {
    result = newState ? BUTTON_WENT_DOWN : BUTTON_WENT_UP;
  }

  button->previousState = newState;

  return result;
}