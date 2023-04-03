#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include <hardware/rtc.h>
#include "rain.h"

/* Critical sections */
static critical_section_t bucket_crit_sec;

float daily_rain = 0.0;
int32_t rain_pulses = 0;
uint64_t bucket_last_ts_usec = 0;
uint64_t bucket_bounce_delta_usec = 100 * 1000;

extern void rain_rtc_timer_cb()
{
  /* called every RTC alarm, which is every 1 minute
   * Check if is midnight in order to reset daily readings
   */
  datetime_t now = {0};

  rtc_get_datetime(&now);

  if (now.hour == 0 && now.min == 0)
  {
    daily_rain = 0.0;
    rain_pulses = 0;
  }
}

extern float rain_get_daily()
{
  return daily_rain;
}

extern float rain_get_rate()
{
  // TODO
  return 0.0;
}

extern int32_t rain_get_pulses()
{
  return rain_pulses;
}

extern void rain_gauge_tick()
{
  uint64_t now = time_us_64();

  if ((now - bucket_last_ts_usec) >= bucket_bounce_delta_usec)
  {
    bucket_last_ts_usec = now;
    critical_section_enter_blocking(&bucket_crit_sec);
    rain_pulses++;
    daily_rain = daily_rain + 0.2;
    critical_section_exit(&bucket_crit_sec);
  }
}

extern bool rain_init()
{
  critical_section_init(&bucket_crit_sec);

  return true;
}