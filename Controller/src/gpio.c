#include "gpio.h"
#include <avr/io.h>

static uint16_t pin_register_address(gpio_port_t port);
static uint16_t ddr_register_address(gpio_port_t port);
static uint16_t port_register_address(gpio_port_t port);
static uint16_t port_fix_offset(gpio_port_t port, uint16_t address);

void gpio_configure_output(gpio_port_t port, gpio_pin_t pin)
{
  if (port < GPIO_PORT_H)
  _SFR_IO8(ddr_register_address(port)) |= (1 << pin);
  else
  _SFR_MEM8(ddr_register_address(port)) |= (1 << pin);
}

void gpio_configure_input(gpio_port_t port, gpio_pin_t pin)
{
  if (port < GPIO_PORT_H)
  _SFR_IO8(ddr_register_address(port)) &= ~(1 << pin);
  else
  _SFR_MEM8(ddr_register_address(port)) &= ~(1 << pin);
}

void gpio_set_pin_value(gpio_port_t port, gpio_pin_t pin, bool enabled)
{
  if (enabled)
    gpio_set_pin(port, pin);
  else
    gpio_reset_pin(port, pin);
}

void gpio_set_pin(gpio_port_t port, gpio_pin_t pin)
{
  if (port < GPIO_PORT_H)
  _SFR_IO8(port_register_address(port)) |= (1 << pin);
  else
  _SFR_MEM8(port_register_address(port)) |= (1 << pin);
}

void gpio_reset_pin(gpio_port_t port, gpio_pin_t pin)
{
  if (port < GPIO_PORT_H)
  _SFR_IO8(port_register_address(port)) &= ~(1 << pin);
  else
  _SFR_MEM8(port_register_address(port)) &= ~(1 << pin);
}

void gpio_toggle_pin(gpio_port_t port, gpio_pin_t pin)
{
  if (port < GPIO_PORT_H)
  _SFR_IO8(port_register_address(port)) ^= (1 << pin);
  else
  _SFR_MEM8(port_register_address(port)) ^= (1 << pin);
}

bool gpio_get_input(gpio_port_t port, gpio_pin_t pin)
{
  if (port < GPIO_PORT_H)
  return _SFR_IO8(pin_register_address(port)) & (1 << pin);
  else
  return _SFR_MEM8(pin_register_address(port)) & (1 << pin);
}

static uint16_t port_fix_offset(gpio_port_t port, uint16_t address)
{
  if (port >= GPIO_PORT_H)
  {
    return address + 0x100 - 3 * GPIO_PORT_H;
  }
  return address;
}

static uint16_t pin_register_address(gpio_port_t port)
{
  return port_fix_offset(port, port * 3);
}

static uint16_t ddr_register_address(gpio_port_t port)
{
  return port_fix_offset(port, port * 3 + 1);
}

static uint16_t port_register_address(gpio_port_t port)
{
  return port_fix_offset(port, port * 3 + 2);
}