#include "arduino/servo.h"
#include "arduino/mega.h"
#include "arduino/gpio.h"
#include <avr/io.h>
#include <stdbool.h>

#define SERVO_ANGLE_0_TIMER_COMP      117
#define SERVO_ANGLE_180_TIMER_COMP    617
#define F_TIMER                       (F_CPU / 64)

void servo_initialize(void)
{
  gpio_configure_output(GPIO_PORT_E, GPIO_PIN_4);

  ICR3 = F_TIMER / 50 / 2;    // 50 Hz, divide by two due to the PWM mode we use
  TCCR3A = (1 << COM3B1);     // Set output when < compare, clear output when > compare value
  TCCR3B = (1 << WGM33);      // Phase and Frequency accurate PWM with ICR as TOP value

  servo_set_angle(90);
}

void servo_enable(void)
{
  // Start timer by setting clock source to F_CPU / 64
  TCCR3B |= (1 << CS30) | (1 << CS31);
}

void servo_disable(void)
{
  // Stop timer by disabling clock source
  TCCR3B &= ~((1 << CS32) | (1 << CS31) | (1 << CS30));
}

void servo_set_angle(uint8_t angle_deg)
{
  uint16_t timerCompareValue = 0;

  if (angle_deg > 180)
  {
    angle_deg = 180;
  }

  timerCompareValue = SERVO_ANGLE_0_TIMER_COMP + (uint32_t) angle_deg * (SERVO_ANGLE_180_TIMER_COMP - SERVO_ANGLE_0_TIMER_COMP) / 180;

  OCR3B = timerCompareValue / 2;
}