#include "sysb/timer.h"
#include "sysb/config.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct 
{
  bool active;
  uint16_t timeLeft_ms;
  uint16_t duration_ms;
  timer_mode_t mode;
  timer_callback_t callback;
} timer_data_t;

static timer_data_t timers[SYSB_MAX_TIMERS];
static uint8_t timerCount;

void timer_initialize(void)
{
}

uint8_t timer_create(timer_mode_t mode, timer_callback_t callback)
{
  if (timerCount >= SYSB_MAX_TIMERS)
  {
    return TIMER_HANDLE_INVALID;
  }

  timers[timerCount].active = false;
  timers[timerCount].timeLeft_ms = 0;
  timers[timerCount].duration_ms = 0;
  timers[timerCount].callback = callback;
  timers[timerCount].mode = mode;

  return timerCount++;
}

void timer_start(uint8_t timerHandle, uint16_t duration_ms)
{
  if (timerHandle >= timerCount)
  {
    return;
  }

  timers[timerHandle].active = true;
  timers[timerHandle].timeLeft_ms = duration_ms;
  timers[timerHandle].duration_ms = duration_ms;
}

void timer_stop(uint8_t timerHandle)
{
  if (timerHandle >= timerCount)
  {
    return;
  }

  timers[timerHandle].active = false;
}

void timer_tick(uint16_t milliseconds)
{
  for (uint8_t i = 0; i < timerCount; i++)
  {
    if (timers[i].active)
    {
      if (timers[i].timeLeft_ms > milliseconds)
      {
        timers[i].timeLeft_ms -= milliseconds;
      }
      else
      {
        timers[i].timeLeft_ms = timers[i].duration_ms;
        timers[i].active = (timers[i].mode == TIMER_MODE_REPEATING);
        if (timers[i].callback != NULL)
        {
          timers[i].callback(i);
        }
      }
    }
  }
}