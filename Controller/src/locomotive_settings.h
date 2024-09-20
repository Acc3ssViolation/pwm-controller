#ifndef LOCOMOTIVE_SETTINGS_H
#define LOCOMOTIVE_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint8_t vMin;         // Voltage output for the lowest speed step.
  uint8_t vMax;         // Voltage output for the highest speed step.
  uint8_t vMid;         // Voltage output for the middle speed step. Ignored when 0.
  uint8_t acc;          // Acceleration value.
  uint8_t dec;          // Deceleration value.
  uint8_t boostPower;   // Voltage to be applied when going from step 0 to step 1 to overcome static motor shenanigans.
  uint8_t revId;        // When set to a valid value, the profile ID of the profile used for the reversed direction settings.
} locomotive_profile_t;

typedef enum
{
  DIRECTION_FOWARD,
  DIRECTION_REVERSED,
} direction_t;

void locomotive_settings_initialize(void);

const locomotive_profile_t *locomotive_settings_get_active(void);

uint8_t locomotive_settings_map_speed(const locomotive_profile_t *settings, uint8_t throttle, direction_t direction);

uint8_t locomotive_settings_apply_speed(const locomotive_profile_t *settings, uint8_t speed, uint8_t targetSpeed, uint8_t delta_ms, direction_t direction);

uint8_t locomotive_settings_get_boost_power(const locomotive_profile_t *settings, direction_t direction);

#endif