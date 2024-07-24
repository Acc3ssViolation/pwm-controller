#ifndef BUTTON_H_
#define BUTTON_H_

#include "gpio.h"
#include <stdbool.h>

#define BUTTON(gpioInfo)  { .gpio = gpioInfo, .previousState = false }

typedef enum
{
  BUTTON_STAYED_UP,
  BUTTON_STAYED_DOWN,
  BUTTON_WENT_UP,
  BUTTON_WENT_DOWN,
} button_state_t;

typedef struct
{
  gpio_info_t gpio;
  bool previousState;
} button_t;

void button_initialize(button_t* button);
button_state_t button_update(button_t* button);


#endif /* BUTTON_H_ */