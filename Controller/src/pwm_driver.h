#ifndef PWM_DRIVER_H
#define PWM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

void pwm_driver_initialize(void);

void pwm_driver_set_enabled(bool enabled);

void pwm_driver_set_duty_cycle(uint8_t dutyCycle);

void pwm_driver_set_reversed(bool reversed);

#endif