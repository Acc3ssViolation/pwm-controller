#include "train.h"
#include "stdlib.h"

void train_initialize(train_t *train, const locomotive_profile_t *profile)
{
  train->profile = profile;
  train->velocity = 0;
  train->state = TRAIN_STATE_STOPPED;
  train->reverser = REVERSER_NEUTRAL;

  train->timer_ms = 0;

  train->input_velocity = 0;
}

void train_apply_input(train_t *train, int16_t throttle)
{
  train->input_velocity = throttle;
}

void train_update(train_t *train,  int16_t delta_ms)
{
  if (abs(train->input_velocity) < abs(train->velocity))
  {
    
  }
  locomotive_settings_apply_speed(train->profile, train->velocity, train->input_velocity, 10, train->reverser);
}