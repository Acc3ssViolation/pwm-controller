#ifndef INPUT_DRIVER_H
#define INPUT_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  INPUT_DIRECTION_IDLE = 0,
  INPUT_DIRECTION_BACKWARDS = -1,
  INPUT_DIRECTION_FORWARDS = 1,
} input_direction_t;

void input_driver_initialize(void);

input_direction_t input_driver_get_direction(void);

uint16_t input_driver_get_throttle(void);

#endif