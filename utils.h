#ifndef _UTILS_H_
#define _UTILS_H_

#include <pico/stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

  extern void float_to_bytes(float val, uint8_t bytes_array[sizeof(float)]);

#ifdef __cplusplus
}
#endif

#endif