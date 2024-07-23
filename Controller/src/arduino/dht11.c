#include "arduino/dht11.h"
#include "arduino/gpio.h"
#include "arduino/mega.h"
#include <string.h>
#include <util/delay.h>
#include <avr/io.h>

static uint8_t wait_for_line_level(gpio_info_t data, bool level);

void dht11_initialize(dht11_t* dht, const gpio_info_t data)
{
  dht->data = data;

  // Set pin high, this is the default data line state
  gpio_set_pin(dht->data.port, dht->data.pin);
}

bool dht11_read(dht11_t* dht, dht11_output_t* output)
{
  uint8_t time_us;
  uint8_t receivedBitCount = 0;
  uint8_t rxBytes[5];
  uint8_t rxBit;
  bool done;

  // Set up Timer 0 for our bit timing uses
  TCCR0A = 0;
  TCCR0B = 0;

  memset(output, 0, sizeof(dht11_output_t));

  // Pull data line down for at least 18 ms
  gpio_configure_output(dht->data.port, dht->data.pin);
  gpio_reset_pin(dht->data.port, dht->data.pin);
  _delay_ms(20);

  // Set pin to input with pull up
  gpio_set_pin(dht->data.port, dht->data.pin);
  gpio_configure_input(dht->data.port, dht->data.pin);

  // Wait for dht to pull line low (20-40 us)
  time_us = wait_for_line_level(dht->data, false);
  // Wait for dht to pull line high (80 us)
  time_us = wait_for_line_level(dht->data, true);
  // Wait for dht to pull line low (80 us)
  time_us = wait_for_line_level(dht->data, false);

  // receive bits!
  done = false;
  for (uint8_t byteIndex = 0; byteIndex < 5; byteIndex++)
  {
    rxBytes[byteIndex] = 0;

    // Bits are received MSB first
    for (int8_t bitIndex = 7; bitIndex >= 0; bitIndex--)
    {
      // wait ~50 us for it to go high
      time_us = wait_for_line_level(dht->data, true);
      // wait for line to go low again
      time_us = wait_for_line_level(dht->data, false);

      if (time_us < 50)
      {
        // 0 is 26-28 us
        rxBit = 0;
      }
      else if (time_us < 100)
      {
        // 1 is 70 us
        rxBit = 1;
      }
      else
      {
        // Line stays high, transmission done
        done = true;
        break;
      }

      // Add bit to byte
      rxBytes[byteIndex] |= (rxBit << bitIndex);

      receivedBitCount++;
    }

    if (done)
    {
      break;
    }
  }

  if (receivedBitCount != 40)
  {
    // Did not receive the expected amount of data
    output->humidity = receivedBitCount;
    return false;
  }

  output->humidity = rxBytes[0];
  output->humidityDecimal = rxBytes[1];
  output->temperature = rxBytes[2];
  output->temperatureDecimal = rxBytes[3];

  if (((rxBytes[0] + rxBytes[1] + rxBytes[2] + rxBytes[3]) & 0xFF) != rxBytes[4])
  {
    // Checksum error
    return false;
  }

  return true;
}

static uint8_t wait_for_line_level(gpio_info_t data, bool level)
{
  uint8_t ticks = 0;

  // Set up timer 0
  TCNT0 = 0;
  TCCR0B = TIMER_PRESCALER_64;  // Prescaler from F_CPU of 16 MHz, gives 4 us per tick

  // Keep polling until some arbitrary timeout value
  while (ticks < 100)
  {
    if (gpio_get_input(data.port, data.pin) == level)
    {
      break;
    }
    ticks = TCNT0;
  }

  // Stop timer
  TCCR0B = TIMER_PRESCALER_DISABLED;

  return ticks * 4;
}