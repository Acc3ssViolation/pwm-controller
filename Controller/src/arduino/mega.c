#include "arduino/mega.h"
#include "arduino/gpio.h"



void arduino_board_init(void)
{
  gpio_configure_output(GPIO_PORT_B, GPIO_PIN_7);
}

void arduino_enable_led(void)
{
  gpio_set_pin(GPIO_PORT_B, GPIO_PIN_7);
}

void arduino_disable_led(void)
{
  gpio_reset_pin(GPIO_PORT_B, GPIO_PIN_7);
}

void arduino_toggle_led(void)
{
  gpio_toggle_pin(GPIO_PORT_B, GPIO_PIN_7);
}
