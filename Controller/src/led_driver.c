#include "platform.h"
#include "led_driver.h"
#include "gpio.h"
#include "timer.h"

typedef struct
{
  gpio_info_t gpio;
  led_mode_t mode;
  bool state;
} led_data_t;

static led_data_t _leds[NR_OF_LEDS] = {0};

static void update(void);

static void led_timer(uint8_t timerHandle);

static void init_led(led_data_t *led, gpio_port_t port, gpio_pin_t pin);

void led_driver_initialize(void)
{
  init_led(&_leds[LED_ERROR], GPIO_PORT_C, GPIO_PIN_3);
  init_led(&_leds[LED_PWM_ON], GPIO_PORT_B, GPIO_PIN_0);
  init_led(&_leds[LED_PC_CONTROL], GPIO_PORT_B, GPIO_PIN_1);

  uint8_t handle = timer_create(TIMER_MODE_REPEATING, led_timer);
  timer_start(handle, 500);
}

void led_driver_set(led_t led, led_mode_t mode)
{
  if (led > NR_OF_LEDS)
  {
    return;
  }

  if (_leds[led].mode == mode)
  {
    return;
  }

  _leds[led].mode = mode;

  update();
}

static void init_led(led_data_t *led, gpio_port_t port, gpio_pin_t pin)
{
  led->gpio.port = port;
  led->gpio.pin = pin;
  led->mode = LED_MODE_DISABLED;
  led->state = false;
  gpio_configure_output(port, pin);
  gpio_reset_pin(port, pin);
}

static void update(void)
{
  for (int8_t i = 0; i < NR_OF_LEDS; i++)
  {
    led_data_t *led = &_leds[i];
    switch (led->mode)
    {
    case LED_MODE_DISABLED:
    {
      led->state = false;
    }
    break;
    case LED_MODE_ON:
    {
      led->state = true;
    }
    break;
    case LED_MODE_BLINK:
    {
      led->state = !led->state;
    }
    break;
    }

    if (led->state)
    {
      gpio_set_pin(led->gpio.port, led->gpio.pin);
    }
    else
    {
      gpio_reset_pin(led->gpio.port, led->gpio.pin);
    }
  }
}

static void led_timer(uint8_t timerHandle)
{
  (void)timerHandle;

  update();
}