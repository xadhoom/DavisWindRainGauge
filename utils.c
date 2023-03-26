#include <string.h>
#include "utils.h"

int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void int32_to_bytes(int32_t val, uint8_t bytes_array[sizeof(int32_t)])
{
  union
  {
    int32_t int32_variable;
    uint8_t temp_array[sizeof(int32_t)];
  } u;

  u.int32_variable = val;

  memcpy(bytes_array, u.temp_array, sizeof(int32_t));
}

void float_to_bytes(float val, uint8_t bytes_array[sizeof(float)])
{
  // Create union of shared memory space
  union
  {
    float float_variable;
    uint8_t temp_array[sizeof(float)];
  } u;

  // Overite bytes of union with float variable
  u.float_variable = val;

  // Assign bytes to input array
  memcpy(bytes_array, u.temp_array, sizeof(float));
}