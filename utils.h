#ifndef _UTILS_H_
#define _UTILS_H_

#include <pico/stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

  extern void float2Bytes(float val, uint8_t *bytes_array);

#ifdef __cplusplus
}
#endif

#endif