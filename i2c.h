#ifndef _I2C_H_
#define _I2C_H_

#include <pico/i2c_slave.h>

#define I2C_IF i2c0
#define I2C_BAUDRATE 100000 // 100 kHz

typedef enum
{
  I2C_COMMAND_SET_RTC,
  I2C_COMMAND_READ_RTC,
  I2C_COMMAND_READ_WIND_SPEED,
  I2C_COMMAND_READ_WIND_DIRECTION,
  I2C_COMMAND_READ_RAIN_RATE,
  I2C_COMMAND_READ_RAIN_DAILY
} i2c_command_t;

typedef enum
{
  I2C_STATE_IDLE,
  I2C_STATE_SET_RTC,
  I2C_STATE_READ_RTC_CMD,
  I2C_STATE_READ_RTC,
  I2C_STATE_READ_WIND_SPEED_CMD,
  I2C_STATE_READ_WIND_SPEED,
  I2C_STATE_READ_WIND_DIRECTION_CMD,
  I2C_STATE_READ_WIND_DIRECTION,
  I2C_STATE_READ_RAIN_RATE_CMD,
  I2C_STATE_READ_RAIN_RATE,
  I2C_STATE_READ_RAIN_DAILY_CMD,
  I2C_STATE_READ_RAIN_DAILY
} i2c_state_machine_t;

#ifdef __cplusplus
extern "C"
{
#endif

  extern void start_i2c_slave(const uint address, const uint sda_pin, const uint scl_pin);
  extern void setup_i2c_slave(const uint address, const uint sda_pin, const uint scl_pin);

#ifdef __cplusplus
}
#endif

#endif