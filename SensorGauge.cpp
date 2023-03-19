#include <stdio.h>
#include <hardware/rtc.h>
#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include <PicoLed.hpp>
#include "low_power.h"
#include "i2c.h"
#include "wind.h"

#define I2C_SLAVE_ADDRESS 0x17
#define I2C_SLAVE_SDA_PIN 0
#define I2C_SLAVE_SCL_PIN 1

#define AWAKE_PIN 11

#define LED_PIN 16
#define LED_LENGTH 1

#define BUCKET_PIN 14
#define BUCKET_IRQ_MASK GPIO_IRQ_EDGE_FALL

#define WIND_PIN 15
#define WIND_IRQ_MASK GPIO_IRQ_EDGE_FALL

int32_t bucket_irq = 0;
uint64_t bucket_last_ts_usec = 0;
uint64_t bucket_bounce_delta_usec = 100 * 1000;

bool rtc_alarm = false;

/* Critical sections */
static critical_section_t rtc_crit_sec;
static critical_section_t bucket_crit_sec;

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
  uint64_t now = time_us_64();
  if ((gpio == BUCKET_PIN) && (events & BUCKET_IRQ_MASK))
  {

    if ((now - bucket_last_ts_usec) >= bucket_bounce_delta_usec)
    {
      bucket_last_ts_usec = now;
      critical_section_enter_blocking(&bucket_crit_sec);
      bucket_irq++;
      critical_section_exit(&bucket_crit_sec);
    }
  }
  else if ((gpio == WIND_PIN) && (events & WIND_IRQ_MASK))
  {
    wind_speed_tick();
  }
}

static void init_gpios(void)
{
  gpio_init(AWAKE_PIN);
  gpio_init(BUCKET_PIN);
  gpio_init(WIND_PIN);

  gpio_pull_up(AWAKE_PIN);
  gpio_pull_up(BUCKET_PIN);
  gpio_pull_up(WIND_PIN);

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
}

static void schedule_blink()
{
  datetime_t now = {0};
  rtc_get_datetime(&now);

  int8_t sec = now.sec + 5;
  if (sec >= 60)
  {
    sec = sec - 60;
  }

  datetime_t alarm = {
      .year = -1,
      .month = -1,
      .day = -1,
      .dotw = -1, // 0 is Sunday, so 3 is Wednesday
      .hour = -1,
      .min = -1,
      .sec = sec};

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

int main()
{
  set_low_power();

  // Re init uart now that clk_peri has changed
  stdio_init_all();

  // init i2c slave interface
  start_i2c_slave(I2C_SLAVE_ADDRESS, I2C_SLAVE_SDA_PIN, I2C_SLAVE_SCL_PIN);

  auto ledStrip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, LED_PIN, LED_LENGTH, PicoLed::FORMAT_GRB);
  signal_startup_with_leds(ledStrip);

  /* Critical sections */
  critical_section_init(&rtc_crit_sec);
  critical_section_init(&bucket_crit_sec);

  /* init gpios */
  init_gpios();

  // Start the Real time clock
  setup_rtc();

  /* init wind stuff */
  if (!wind_init())
  {
    blink_led(ledStrip, PicoLed::RGB(255, 0, 0), 25);
  }

  schedule_blink();

  while (true)
  {
    if (bucket_irq || wind_pulses)
    {
      blink_led(ledStrip, PicoLed::RGB(0, 255, 255), 25);
    }

    if (rtc_alarm)
    {
      critical_section_enter_blocking(&rtc_crit_sec);
      rtc_alarm = false;
      critical_section_exit(&rtc_crit_sec);
      blink_led(ledStrip, PicoLed::RGB(255, 255, 0), 25);
      schedule_blink();
    }

    __wfi();
  }
}