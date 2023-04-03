#include <stdio.h>
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include <pico/multicore.h>
#include <PicoLed.hpp>
#include "low_power.h"
#include "i2c.h"
#include "wind.h"
#include "rain.h"

#define I2C_SLAVE_ADDRESS 0x17
#define I2C_SLAVE_SDA_PIN 0
#define I2C_SLAVE_SCL_PIN 1

#define LED_PIN 16
#define LED_LENGTH 1

#define HB_BLINK_INTVL_SEC 5

#define BUCKET_PIN 14
#define BUCKET_IRQ_MASK GPIO_IRQ_EDGE_FALL
// #define BUCKET_PIN_PULL_UP

#define WIND_PIN 15
#define WIND_DIRECTION_PIN 29      // this is an analog input
#define WIND_DIRECTION_ADC_INPUT 3 // this is an analog input
#define WIND_IRQ_MASK GPIO_IRQ_EDGE_FALL
// #define WIND_PIN_PULL_UP

bool rtc_alarm = false;

/* Critical sections */
static critical_section_t rtc_crit_sec;

/* timers */
struct repeating_timer hb_blink_timer;

/* */
static bool hb_blink = false;

static void shutdown_leds(PicoLed::PicoLedController ledStrip)
{
  ledStrip.setBrightness(0);
  ledStrip.fill(PicoLed::RGB(0, 0, 0));
  ledStrip.show();
  sleep_ms(5);
}

static void blink_led(PicoLed::PicoLedController ledStrip, PicoLed::Color color, uint32_t ms)
{
  ledStrip.setBrightness(20);
  ledStrip.fill(color);
  ledStrip.show();
  sleep_ms(ms);
  shutdown_leds(ledStrip);
}

void gpio_callback(uint gpio, uint32_t events)
{
  if ((gpio == BUCKET_PIN) && (events & BUCKET_IRQ_MASK))
  {
    rain_gauge_tick();
  }
  else if ((gpio == WIND_PIN) && (events & WIND_IRQ_MASK))
  {
    wind_speed_tick();
  }
}

static void init_gpios(void)
{
  gpio_init(BUCKET_PIN);
  gpio_init(WIND_PIN);

#ifdef BUCKET_PIN_PULL_UP
  gpio_pull_up(BUCKET_PIN);
#endif
#ifdef WIND_PIN_PULL_UP
  gpio_pull_up(WIND_PIN);
#endif

  gpio_set_irq_enabled(BUCKET_PIN, BUCKET_IRQ_MASK, true);
  gpio_set_irq_enabled(WIND_PIN, WIND_IRQ_MASK, true);

  gpio_set_irq_callback(&gpio_callback);
  irq_set_priority(IO_IRQ_BANK0, 0xff);
  irq_set_enabled(IO_IRQ_BANK0, true);
}

static void signal_startup_with_leds(PicoLed::PicoLedController ledStrip)
{

  blink_led(ledStrip, PicoLed::RGB(255, 0, 0), 500);
  blink_led(ledStrip, PicoLed::RGB(0, 255, 0), 500);
  blink_led(ledStrip, PicoLed::RGB(0, 0, 255), 500);
}

static void rtc_alarm_callback(void)
{
  critical_section_enter_blocking(&rtc_crit_sec);
  rtc_alarm = true;
  critical_section_exit(&rtc_crit_sec);

  rain_rtc_timer_cb();
}

static void schedule_rtc_every_1_minute()
{
  datetime_t alarm = {
      .year = -1,
      .month = -1,
      .day = -1,
      .dotw = -1, // 0 is Sunday, so 3 is Wednesday
      .hour = -1,
      .min = -1,
      .sec = 0};

  rtc_set_alarm(&alarm, &rtc_alarm_callback);
}

static void setup_rtc()
{
  // Start on Wednesday 13th January 2021 11:20:00
  datetime_t t = {
      .year = 2020,
      .month = 01,
      .day = 13,
      .dotw = 3, // 0 is Sunday, so 3 is Wednesday
      .hour = 11,
      .min = 20,
      .sec = 00};

  // Start the RTC
  rtc_init();
  rtc_set_datetime(&t);
}

static void init_adc_inputs()
{
  adc_init();
  adc_gpio_init(WIND_DIRECTION_PIN);
}

static bool hb_timer_callback(struct repeating_timer *t)
{
  hb_blink = true;

  return true;
}

static void schedule_blink_hb(PicoLed::PicoLedController ledStrip)
{
  bool res = add_repeating_timer_ms(-(HB_BLINK_INTVL_SEC * 1000), hb_timer_callback, NULL, &hb_blink_timer);

  if (!res)
  {
    blink_led(ledStrip, PicoLed::RGB(0, 0, 255), 25);
  }
}

void core1_entry()
{
  //  init i2c slave interface
  start_i2c_slave(I2C_SLAVE_ADDRESS, I2C_SLAVE_SDA_PIN, I2C_SLAVE_SCL_PIN);
}

int main()
{
  int32_t prev_rain_pulses, prev_wind_pulses = 0;

  set_low_power();

  // Re init uart now that clk_peri has changed
  stdio_init_all();

  // Start i2c over core 1
  // see: https://github.com/raspberrypi/pico-sdk/issues/1102
  multicore_launch_core1(core1_entry);

  auto ledStrip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, LED_PIN, LED_LENGTH, PicoLed::FORMAT_GRB);
  signal_startup_with_leds(ledStrip);

  /* Critical sections */
  critical_section_init(&rtc_crit_sec);

  /* init gpios */
  init_gpios();

  /* init adc */
  init_adc_inputs();

  /* enable onboard temp adc input*/
  adc_set_temp_sensor_enabled(true);

  // Start the Real time clock
  setup_rtc();

  /* init wind stuff */
  if (!wind_init(WIND_DIRECTION_ADC_INPUT))
  {
    blink_led(ledStrip, PicoLed::RGB(255, 0, 0), 25);
  }

  schedule_rtc_every_1_minute();

  schedule_blink_hb(ledStrip);

  while (true)
  {
    if ((rain_get_pulses() > prev_rain_pulses) || (wind_pulses > prev_wind_pulses))
    {
      blink_led(ledStrip, PicoLed::RGB(0, 255, 255), 25);
    }
    prev_rain_pulses = rain_get_pulses();
    prev_wind_pulses = wind_pulses;

    if (hb_blink)
    {
      hb_blink = false;
      blink_led(ledStrip, PicoLed::RGB(255, 255, 0), 25);
    }

    __wfi();
  }
}