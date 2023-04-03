#ifndef _RAIN_H_
#define _RAIN_H_

#ifdef __cplusplus
extern "C"
{
#endif

  extern float rain_get_daily();
  extern float rain_get_rate();
  extern int32_t rain_get_pulses();
  extern void rain_rtc_timer_cb();
  extern void rain_gauge_tick();
  extern bool rain_init();

#ifdef __cplusplus
}
#endif

#endif