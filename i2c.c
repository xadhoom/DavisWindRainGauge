#include <string.h>
#include <hardware/rtc.h>
#include <pico/stdlib.h>
#include <pico/i2c_slave.h>
#include "i2c.h"
#include "wind.h"
#include "utils.h"

static uint8_t i2c_state = I2C_STATE_IDLE;

/*
 * RTC: 2 bytes for year, 1 for others
 */
static struct
{
  uint8_t mem[8];
  uint8_t address;
} rtc_setting_ctx;

static struct
{
  uint8_t mem[4];
  uint8_t address;
} wind_speed_ctx;

static void read_windspeed_into_i2c_mem()
{
  float2Bytes(wind_speed, &wind_speed_ctx.mem[0]);
  wind_speed_ctx.address = 0;
}

static void write_i2c_mem_into_rtc()
{
  uint8_t year_msb, year_lsb, month, day, minute, second;

  datetime_t dt = {
      .year = rtc_setting_ctx.mem[1] + (rtc_setting_ctx.mem[0] << 8),
      .month = rtc_setting_ctx.mem[2],
      .day = rtc_setting_ctx.mem[3],
      .dotw = rtc_setting_ctx.mem[4], // 0 is Sunday, so 3 is Wednesday
      .hour = rtc_setting_ctx.mem[5],
      .min = rtc_setting_ctx.mem[6],
      .sec = rtc_setting_ctx.mem[7]};

  rtc_set_datetime(&dt);
}

static void read_rtc_into_i2c_mem()
{
  datetime_t now;
  uint8_t year_msb, year_lsb;
  rtc_get_datetime(&now);

  year_msb = now.year >> 8;
  year_lsb = now.year & 0xff;

  rtc_setting_ctx.address = 0;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = year_msb;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = year_lsb;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = now.month;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = now.day;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = now.dotw;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = now.hour;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = now.min;
  rtc_setting_ctx.address++;
  rtc_setting_ctx.mem[rtc_setting_ctx.address] = now.sec;
  rtc_setting_ctx.address = 0;
}

static void start_i2c_command(i2c_inst_t *i2c)
{
  uint8_t command;

  // first byte is the command
  command = i2c_read_byte_raw(i2c);

  switch (command)
  {
  case I2C_COMMAND_SET_RTC:
    i2c_state = I2C_STATE_SET_RTC;
    rtc_setting_ctx.address = 0;
    memset(rtc_setting_ctx.mem, 0, sizeof(rtc_setting_ctx.mem));
    break;
  case I2C_COMMAND_READ_RTC:
    i2c_state = I2C_STATE_READ_RTC_CMD;
    read_rtc_into_i2c_mem();
    break;
  case I2C_COMMAND_READ_WIND_SPEED:
    i2c_state = I2C_STATE_READ_WIND_SPEED_CMD;
    read_windspeed_into_i2c_mem();
    break;
  default:
    break;
  }
}

static void i2c_command_continuation(i2c_inst_t *i2c)
{
  switch (i2c_state)
  {
  case I2C_STATE_SET_RTC:
    rtc_setting_ctx.mem[rtc_setting_ctx.address] = i2c_read_byte_raw(i2c);
    rtc_setting_ctx.address++;
    break;
  case I2C_STATE_READ_RTC:
    i2c_write_byte_raw(i2c, rtc_setting_ctx.mem[rtc_setting_ctx.address]);
    rtc_setting_ctx.address++;
    break;
  case I2C_STATE_READ_WIND_SPEED:
    i2c_write_byte_raw(i2c, wind_speed_ctx.mem[wind_speed_ctx.address]);
    wind_speed_ctx.address++;
    break;
  default:
    break;
  }
}

static void i2c_handle_finish(i2c_inst_t *i2c)
{
  switch (i2c_state)
  {
  case I2C_STATE_SET_RTC:
    write_i2c_mem_into_rtc();
    rtc_setting_ctx.address = 0;
    break;
  case I2C_STATE_READ_RTC:
    rtc_setting_ctx.address = 0;
    break;
  case I2C_STATE_READ_RTC_CMD:
    i2c_state = I2C_STATE_READ_RTC;
    rtc_setting_ctx.address = 0;
    return;
    break;
  case I2C_STATE_READ_WIND_SPEED:
    wind_speed_ctx.address = 0;
    break;
  case I2C_STATE_READ_WIND_SPEED_CMD:
    i2c_state = I2C_STATE_READ_WIND_SPEED;
    wind_speed_ctx.address = 0;
    return;
    break;
  default:
    break;
  }

  i2c_state = I2C_STATE_IDLE;
}

static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{

  switch (event)
  {
  case I2C_SLAVE_RECEIVE:
    if (i2c_state == I2C_STATE_IDLE)
    {
      start_i2c_command(i2c);
    }
    else
    {
      i2c_command_continuation(i2c);
    }
    break;
  case I2C_SLAVE_REQUEST:
    i2c_command_continuation(i2c);
    break;
  case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
    i2c_handle_finish(i2c);
    break;
  default:
    break;
  }
}

extern void start_i2c_slave(const uint address, const uint sda_pin, const uint scl_pin)
{
  return setup_i2c_slave(address, sda_pin, scl_pin);
};

extern void setup_i2c_slave(const uint address, const uint sda_pin, const uint scl_pin)
{
  gpio_init(sda_pin);
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);

  gpio_init(scl_pin);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(scl_pin);

  i2c_init(I2C_IF, I2C_BAUDRATE);
  // configure I2C interface for slave mode
  i2c_slave_init(I2C_IF, address, &i2c_slave_handler);
}
