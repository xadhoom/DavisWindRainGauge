#ifndef _WIND_H_
#define _WIND_H_

#define WIND_SAMPLER_SECS 3
#define WIND_DIR_SAMPLER_SECS 1

extern float wind_speed;
extern int32_t wind_direction;
extern int32_t wind_pulses;

#ifdef __cplusplus
extern "C"
{
#endif

  extern void wind_speed_tick();
  extern bool wind_init(uint8_t adc_input_nr);

#ifdef __cplusplus
}
#endif

#endif