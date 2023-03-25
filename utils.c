#include <string.h>
#include "utils.h"

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