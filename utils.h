#ifndef _UTILS_H_
#define _UTILS_H_

#include <pico/stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

  extern int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
  extern void int32_to_bytes(int32_t val, uint8_t bytes_array[sizeof(int32_t)]);
  extern void float_to_bytes(float val, uint8_t bytes_array[sizeof(float)]);

#ifdef __cplusplus
}
#endif

#endif