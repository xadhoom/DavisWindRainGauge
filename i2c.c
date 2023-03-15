#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#include "i2c.h"

static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
  switch (event)
  {
  case I2C_SLAVE_RECEIVE:
    break;
  case I2C_SLAVE_REQUEST:
    break;
  case I2C_SLAVE_FINISH:
    break;
  }
}

extern void setup_i2c_slave(const uint address, const uint sda_pin, const uint scl_pin)
{
  gpio_init(sda_pin);
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);

  gpio_init(scl_pin);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(scl_pin);

  i2c_init(i2c0, I2C_BAUDRATE);
  // configure I2C0 for slave mode
  i2c_slave_init(i2c0, address, &i2c_slave_handler);
}
