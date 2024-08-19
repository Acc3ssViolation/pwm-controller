#include "stdint.h"
#include "stdbool.h"

typedef enum
{
  LED_ERROR,
  LED_PWM_ON,
  LED_PC_CONTROL,
  NR_OF_LEDS,
} led_t;

typedef enum
{
  LED_MODE_DISABLED,
  LED_MODE_ON,
  LED_MODE_BLINK,
} led_mode_t;

void led_driver_initialize(void);

void led_driver_set(led_t led, led_mode_t mode);