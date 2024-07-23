#ifndef DHT11_H_
#define DHT11_H_

#include "arduino/gpio.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct  
{
  gpio_info_t data;
} dht11_t;

typedef struct  
{
  uint8_t humidity;
  uint8_t humidityDecimal;
  uint8_t temperature;
  uint8_t temperatureDecimal;
} dht11_output_t;

void dht11_initialize(dht11_t* dht, const gpio_info_t data);

bool dht11_read(dht11_t* dht, dht11_output_t* output);

#endif