#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include "wind.h"

#define MPH_CONV_CONSTANT 1.60934

int32_t wind_pulses = 0;
float wind_speed = 0;
uint64_t wind_last_ts_usec = 0;
uint64_t wind_bounce_delta_usec = 20 * 1000;
struct repeating_timer wind_speed_timer;

/* critical sections */
static critical_section_t wind_crit_sec;

static bool windspeed_timer_callback(struct repeating_timer *t)
{
  /*
   * Davis reports that 1600 rotations hour = 1 mph
   * so convert to mp/h using the Davis formula V=P(2.25/T)
   * V = P(2.25/T)
   * where:
   * V is speed in mph
   * P nr of pulses per sample period
   * T is the sample period in seconds
   */

  critical_section_enter_blocking(&wind_crit_sec);
  wind_speed = (wind_pulses * (2.25 / WIND_SAMPLER_SECS)) * MPH_CONV_CONSTANT;
  wind_pulses = 0;
  critical_section_exit(&wind_crit_sec);

  return true;
}

extern void wind_speed_tick()
{
  uint64_t now = time_us_64();

  if ((now - wind_last_ts_usec) >= wind_bounce_delta_usec)
  {
    wind_last_ts_usec = now;
    critical_section_enter_blocking(&wind_crit_sec);
    wind_pulses++;
    critical_section_exit(&wind_crit_sec);
  }
}

extern bool wind_init()
{
  critical_section_init(&wind_crit_sec);

  return add_repeating_timer_ms(WIND_SAMPLER_SECS * 1000, windspeed_timer_callback, NULL, &wind_speed_timer);
}