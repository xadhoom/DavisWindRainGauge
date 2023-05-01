#include <math.h>
#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include <hardware/rtc.h>
#include "rain.h"

/* how many mm on rain for each spoon tip */
#define SPOON_SIZE 0.2

/* Critical sections */
static critical_section_t bucket_crit_sec;

float daily_rain = 0.0;
float rain_rate = 0.0;
int32_t rain_pulses = 0;
/* last bucket tip timestamp, used for rate calcs */
uint64_t rate_last_tip_usec = 0;
/* debounce vars */
uint64_t bucket_last_ts_usec = 0;
uint64_t bucket_bounce_delta_usec = 100 * 1000;

/*
 * 15 minutes is defined by the U.S. National Weather Service as intervening time
 * upon which one rain "event" is considered separate from another rain "event".
 */
#define RAIN_15M_EVENT_MS 15 * 60 * 1000
static alarm_id_t rate_15_min_alarm = -1;
/*
  Using a secondary alarm to smooth out rate when tipping stops,
  because event stops or rain diminishes.
  The idea after each tip is to start another timer with the same latest delta,
  that will be cancelled if a tip arrives in a shorter interval, otherwise
  will recalculate the rate based on the last tip, which will smooth
  out the rate progressively until the end of the rain event.
*/
static alarm_id_t secondary_rate_alarm = -1;
uint64_t secondary_rate_alarm_next_msec = 0;

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
  return rain_rate;
}

extern int32_t rain_get_pulses()
{
  return rain_pulses;
}

static int64_t rain_rate_reset(alarm_id_t id, void *user_data)
{
  if (secondary_rate_alarm > 0)
  {
    cancel_alarm(secondary_rate_alarm);
  }

  rain_rate = 0.0;
  rate_last_tip_usec = 0;
  secondary_rate_alarm_next_msec = 0;
  rate_15_min_alarm = -1;
  secondary_rate_alarm = -1;

  return 0; // do not reschedule the alarm
}

static float compute_rate(uint64_t now, uint64_t last_tip_usec)
{
  float rate;
  uint64_t delta_msec;
  uint64_t hour_msec = 60 * 60 * 1000;

  delta_msec = round((now - last_tip_usec) / 1000);
  rate = (hour_msec / delta_msec) * SPOON_SIZE;

  return rate;
}

static int64_t secondary_rain_rate_timer(alarm_id_t id, void *user_data)
{
  rain_rate = compute_rate(time_us_64(), rate_last_tip_usec);

  // reschedule for same amount. funnily while the alarm interval
  // in expressed in msec when you add it, the return val is in usec.
  return secondary_rate_alarm_next_msec * 1000;
}

static void rain_compute_new_rate()
{
  if (rate_last_tip_usec == 0)
  {
    // no previous tip in this rain event, init it
    rate_last_tip_usec = time_us_64();
    return;
  }
  if (secondary_rate_alarm > 0)
  {
    cancel_alarm(secondary_rate_alarm);
  }

  uint64_t now = time_us_64();
  secondary_rate_alarm_next_msec = round((now - rate_last_tip_usec) / 1000);
  rain_rate = compute_rate(now, rate_last_tip_usec);
  rate_last_tip_usec = now;
  secondary_rate_alarm = add_alarm_in_ms(secondary_rate_alarm_next_msec, &secondary_rain_rate_timer, NULL, false);
}

extern void rain_gauge_tick()
{
  uint64_t now = time_us_64();

  if ((now - bucket_last_ts_usec) >= bucket_bounce_delta_usec)
  {
    bucket_last_ts_usec = now;
    critical_section_enter_blocking(&bucket_crit_sec);
    rain_pulses++;
    daily_rain = daily_rain + SPOON_SIZE;
    critical_section_exit(&bucket_crit_sec);

    if (rate_15_min_alarm > 0)
    {
      // we have an ongoing 15min timer, reset it
      cancel_alarm(rate_15_min_alarm);
      rate_15_min_alarm = add_alarm_in_ms(RAIN_15M_EVENT_MS, &rain_rate_reset, NULL, false);
    }
    else
    {
      // no 15 min timer scheduled, this is a new rain event, start it
      rain_rate_reset(0, NULL); // just in case
      rate_15_min_alarm = add_alarm_in_ms(RAIN_15M_EVENT_MS, &rain_rate_reset, NULL, false);
    }

    rain_compute_new_rate();
  }
}

extern bool rain_init()
{
  critical_section_init(&bucket_crit_sec);

  return true;
}