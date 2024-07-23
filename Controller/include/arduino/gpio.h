#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  GPIO_PORT_A,
  GPIO_PORT_B,
  GPIO_PORT_C,
  GPIO_PORT_D,
  GPIO_PORT_E,
  GPIO_PORT_F,
  GPIO_PORT_G,
  GPIO_PORT_H
} gpio_port_t;

typedef enum
{
  GPIO_PIN_0,
  GPIO_PIN_1,
  GPIO_PIN_2,
  GPIO_PIN_3,
  GPIO_PIN_4,
  GPIO_PIN_5,
  GPIO_PIN_6,
  GPIO_PIN_7,
} gpio_pin_t;

typedef struct  
{
  gpio_port_t port;
  gpio_pin_t pin;
} gpio_info_t;

void gpio_configure_output(gpio_port_t port, gpio_pin_t pin);
void gpio_configure_input(gpio_port_t port, gpio_pin_t pin);
void gpio_set_pin_value(gpio_port_t port, gpio_pin_t pin, bool enabled);
void gpio_set_pin(gpio_port_t port, gpio_pin_t pin);
void gpio_reset_pin(gpio_port_t port, gpio_pin_t pin);
void gpio_toggle_pin(gpio_port_t port, gpio_pin_t pin);
bool gpio_get_input(gpio_port_t port, gpio_pin_t pin);

#endif /* GPIO_H_ */