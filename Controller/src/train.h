#ifndef TRAIN_H
#define TRAIN_H

#include <stdint.h>
#include <stdbool.h>

#include "locomotive_settings.h"

typedef enum
{
  TRAIN_STATE_STOPPED = 0,
  TRAIN_STATE_START_BOOST,
  TRAIN_STATE_ACCELERATING,
  TRAIN_STATE_MOVING,
  TRAIN_STATE_DECELERATING,
  NR_OF_TRAIN_STATES,
} train_state_t;

typedef enum
{
  REVERSER_NEUTRAL = 0,
  REVERSER_FORWARDS = 1,
  REVERSER_BACKWARDS = -1,
} reverser_t;

typedef struct
{
  const locomotive_profile_t *profile;
  int16_t velocity;
  reverser_t reverser;
  train_state_t state;

  int16_t input_velocity;
  int16_t timer_ms;
} train_t;

void train_initialize(train_t *train, const locomotive_profile_t *profile);

void train_apply_input(train_t *train, int16_t throttle);

void train_update(train_t *train, int16_t delta_ms);

#endif