#include <string.h>
#include "utils.h"

void float2Bytes(float val, uint8_t *bytes_array)
{
  // Create union of shared memory space
  union
  {
    float float_variable;
    uint8_t temp_array[4];
  } u;

  // Overite bytes of union with float variable
  u.float_variable = val;

  // Assign bytes to input array
  memcpy(bytes_array, u.temp_array, 4);
}