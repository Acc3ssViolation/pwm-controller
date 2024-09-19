#ifndef LOCOMOTIVE_SETTINGS_H
#define LOCOMOTIVE_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint8_t vMin;
  uint8_t vMax;
  uint8_t vMid;
  uint8_t acc;
  uint8_t dec;
} locomotive_profile_t;

void locomotive_settings_initialize(void);

const locomotive_profile_t *locomotive_settings_get_active(void);

uint8_t locomotive_settings_map_speed(const locomotive_profile_t *settings, uint8_t throttle);

uint8_t locomotive_settings_apply_speed(const locomotive_profile_t *settings, uint8_t speed, uint8_t targetSpeed, uint8_t delta_ms);

#endif