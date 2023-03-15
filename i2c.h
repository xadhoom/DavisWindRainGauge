#ifndef _I2C_H_
#define _I2C_H_

#ifdef __cplusplus
extern "C"
{
#endif

  static const uint I2C_BAUDRATE = 100000; // 100 kHz
  extern void setup_i2c_slave(const uint address, const uint sda_pin, const uint scl_pin);

#ifdef __cplusplus
}
#endif

#endif