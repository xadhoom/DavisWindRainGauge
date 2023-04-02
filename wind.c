#include <hardware/adc.h>
#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include "wind.h"
#include "utils.h"

#define MPH_CONV_CONSTANT 1.60934

uint8_t wind_adc_input_nr;
int32_t wind_pulses = 0;
float wind_speed = 0;
int32_t wind_direction;
uint64_t wind_last_ts_usec = 0;
uint64_t wind_bounce_delta_usec = 20 * 1000;

struct repeating_timer wind_speed_timer;
struct repeating_timer wind_direction_timer;

/* critical sections */
static critical_section_t wind_crit_sec;

static void wind_read_direction()
{
  adc_select_input(wind_adc_input_nr);
  uint16_t vane_reading = adc_read();

  // from pico-sdk docs:
  // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
  // const float conversion_factor = 3.3f / (1 << 12);

  // map 0-4095 to 0-360
  wind_direction = map(vane_reading, 0, 4095, 0, 360);
}

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

static bool winddirection_timer_callback(struct repeating_timer *t)
{
  wind_read_direction();

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

extern bool wind_init(uint8_t adc_input_nr)
{
  critical_section_init(&wind_crit_sec);

  wind_adc_input_nr = adc_input_nr;

  /* using negative times here to start the timer on start of the callbacks
   * and not between the end of one callback to the other.
   */
  bool t1_res = add_repeating_timer_ms(-(WIND_SAMPLER_SECS * 1000), windspeed_timer_callback, NULL, &wind_speed_timer);
  bool t2_res = add_repeating_timer_ms(-(WIND_DIR_SAMPLER_SECS * 1000), winddirection_timer_callback, NULL, &wind_direction_timer);

  if (t1_res && t2_res)
  {
    return true;
  }
  return false;
}