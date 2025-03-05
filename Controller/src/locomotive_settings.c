#include "locomotive_settings.h"
#include "commands.h"

#include <stddef.h>
#include <stdlib.h>

#define NR_OF_PROFILES 3
#define NO_PROFILE     255

static locomotive_profile_t m_defaultProfile = {
  .vMin = 0,
  .vMax = 255,
  .vMid = 127,
  .acc = 0,
  .boostPower = 0,
};
static locomotive_profile_t m_profiles[NR_OF_PROFILES] = {0};
static uint8_t m_activeProfile = NO_PROFILE;

static void set_profile_command(const char *arguments, uint8_t length, const command_functions_t *output);
static void get_profile_command(const char *arguments, uint8_t length, const command_functions_t *output);
static void apply_profile_command(const char *arguments, uint8_t length, const command_functions_t *output);

static const command_t m_commandSetProfile = {
  .prefix = "PR+SET",
  .summary = "Sets the given profile's parameters.",
  .handler = set_profile_command,
};

static const command_t m_commandGetProfile = {
  .prefix = "PR+GET",
  .summary = "Gets the given profile's parameters.",
  .handler = get_profile_command,
};

static const command_t m_commandApplyProfile = {
  .prefix = "PR+ACTIVE",
  .summary = "Gets or sets the active profile index. 255 indicates no profile.",
  .handler = apply_profile_command,
};

static inline int16_t linear_iterp(int16_t from, int16_t to, int16_t fraction)
{
  // from + (to - from) * (fraction / 255)
  // from + (to * fraction - from * fraction) / 255
  return from + (to * fraction - from * fraction) / 255;
}

void locomotive_settings_initialize(void)
{
  commands_register(&m_commandGetProfile);
  commands_register(&m_commandSetProfile);
  commands_register(&m_commandApplyProfile);

  // Kato ED75
  m_profiles[0].vMin = 68;
  m_profiles[0].vMid = 0;
  m_profiles[0].vMax = 155;
  m_profiles[0].acc = 3;
  m_profiles[0].dec = 3;

  // Kato C11
  m_profiles[1].vMin = 15;
  m_profiles[1].vMid = 0;
  m_profiles[1].vMax = 120;
  m_profiles[1].acc = 3;
  m_profiles[1].dec = 3;
  m_profiles[1].boostPower = 20;

  // Tomix DE10
  m_profiles[2].vMin = 34;
  m_profiles[2].vMid = 0;
  m_profiles[2].vMax = 110;
  m_profiles[2].acc = 3;
  m_profiles[2].dec = 3;
  m_profiles[2].boostPower = 40;

  m_activeProfile = 1;
}

const locomotive_profile_t *locomotive_settings_get_active(void)
{
  if (m_activeProfile < NR_OF_PROFILES) 
  {
    return &m_profiles[m_activeProfile];
  }
  return &m_defaultProfile;
}

uint8_t locomotive_settings_map_speed(const locomotive_profile_t *settings, uint8_t throttle, direction_t direction)
{
  if (throttle == 0)
  {
    return 0;
  }

  if (settings->vMin == 0 || settings->vMax == 0)
  {
    return throttle;
  }

  if (settings->vMid != 0)
  {
    if (throttle <= 127)
    {
      return linear_iterp(settings->vMin, settings->vMid, throttle);
    }
    else
    {
      return linear_iterp(settings->vMid, settings->vMax, throttle);
    }
  }
  else
  {
    return linear_iterp(settings->vMin, settings->vMax, throttle);
  }
}

static inline int16_t milliseconds_per_step(int16_t acc, int16_t steps)
{
  // Prevent overflow of 16 bit ints
  if (acc > 36)
  {
    acc = 36;
  }
  return acc * 896 / steps;
}

uint8_t locomotive_settings_apply_speed(const locomotive_profile_t *settings, uint8_t speed, uint8_t targetSpeed, uint8_t delta_ms, direction_t direction)
{
  // 128 speed steps interpretation
  // According to DCC docs we take N seconds per step where N = acc * 0.896 / steps
  if (speed < targetSpeed && settings->acc != 0)
  {
    int16_t step_ms = milliseconds_per_step(settings->acc, 256);
    if (delta_ms > step_ms)
    {
      uint8_t delta = delta_ms / step_ms;
      if (targetSpeed - speed > delta)
      {
        return speed + delta;
      }
      else
      {
        return targetSpeed;
      }
    }
    else
    {
      // TODO: We should actually delay the speed change to some follow-up step
      return speed + 1;
    }
  }
  else if (speed > targetSpeed && settings->dec != 0)
  {
    int16_t step_ms = milliseconds_per_step(settings->dec, 256);
    if (delta_ms > step_ms)
    {
      uint8_t delta = delta_ms / step_ms;
      if (speed - targetSpeed > delta)
      {
        return speed - delta;
      }
      else
      {
        return targetSpeed;
      }
    }
    else
    {
      // TODO: We should actually delay the speed change to some follow-up step
      return speed - 1;
    }
  }
  else
  {
    return targetSpeed;
  }
}

uint8_t locomotive_settings_get_boost_power(const locomotive_profile_t *settings, direction_t direction)
{
  return settings->boostPower;
}

static void set_profile_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  uint8_t profileIndex;
  if (false == commands_get_u8(arguments, length, 0, &profileIndex) || profileIndex >= NR_OF_PROFILES)
  {
    output->writeln(ERR_WITH_REASON("Invalid profile index"));
    return;
  }

  uint8_t params[7];
  for (int8_t i = 0; i < (sizeof(params) / sizeof(params[0])); i++)
  {
    if (false == commands_get_u8(arguments, length, i + 1, &params[i]))
    {
      output->writeln_format(ERR_WITH_REASON("Invalid argument at index %u"), i);
      return;
    }
  }

  locomotive_profile_t *prof = &m_profiles[profileIndex];
  prof->vMin = params[0];
  prof->vMid = params[1];
  prof->vMax = params[2];
  prof->acc = params[3];
  prof->dec = params[4];
  prof->boostPower = params[5];
  prof->boostPower = params[6];

  output->writeln_format(OK_WITH_RESULT("Updated profile %u"), profileIndex);
}

static void get_profile_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  uint8_t profileIndex;
  if (false == commands_get_u8(arguments, length, 0, &profileIndex) || profileIndex >= NR_OF_PROFILES)
  {
    output->writeln(ERR_WITH_REASON("Invalid profile index"));
    return;
  }

  const locomotive_profile_t *prof = &m_profiles[profileIndex];
  output->writeln(COM_OK "+");
  output->writeln_format("id:%u+", profileIndex);
  output->writeln_format("vMin:%u+", prof->vMin);
  output->writeln_format("vMid:%u+", prof->vMid);
  output->writeln_format("vMax:%u+", prof->vMax);
  output->writeln_format("acc:%u+", prof->acc);
  output->writeln_format("dec:%u+", prof->dec);
  output->writeln_format("bPower:%u+", prof->boostPower);
}

static void apply_profile_command(const char *arguments, uint8_t length, const command_functions_t *output)
{
  uint8_t profileIndex = 0;
  if (commands_get_u8(arguments, length, 0, &profileIndex))
  {
    if (profileIndex >= NR_OF_PROFILES)
    {
      profileIndex = NO_PROFILE;
    }
    m_activeProfile = profileIndex;
  }

  output->writeln_format(OK_WITH_RESULT("active:%u"), m_activeProfile);
}