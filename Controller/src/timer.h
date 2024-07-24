#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

#define TIMER_HANDLE_INVALID    (0xFF)

typedef enum
{
  TIMER_MODE_SINGLE,
  TIMER_MODE_REPEATING,
} timer_mode_t;

typedef void (*timer_callback_t)(uint8_t timerHandle);

void timer_initialize(void);

uint8_t timer_create(timer_mode_t mode, timer_callback_t callback);

void timer_start(uint8_t timerHandle, uint16_t duration_ms);

void timer_stop(uint8_t timerHandle);

void timer_tick(uint16_t milliseconds);


#endif /* TIMER_H_ */