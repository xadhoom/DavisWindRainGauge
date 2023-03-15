#include <hardware/clocks.h>
#include <hardware/rosc.h>
#include <hardware/structs/scb.h>
#include <pico/stdlib.h>
#include <pico/critical_section.h>
#include <pico/sleep.h>
#include <PicoLed.hpp>
#include "low_power.h"
#include "i2c.h"

#define I2C_SLAVE_ADDRESS 0x17
#define I2C_SLAVE_SDA_PIN 12
#define I2C_SLAVE_SCL_PIN 13

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

int32_t wind_irq = 0;
uint64_t wind_last_ts_usec = 0;
uint64_t wind_bounce_delta_usec = 20 * 1000;

bool sleep_alarm = false;

/* Critical sections */
static critical_section_t sleep_crit_sec;
static critical_section_t bucket_crit_sec;
static critical_section_t wind_crit_sec;

void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig)
{
  // Re-enable ring Oscillator control
  rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

  // reset procs back to default
  scb_hw->scr = scb_orig;
  clocks_hw->sleep_en0 = clock0_orig;
  clocks_hw->sleep_en1 = clock1_orig;

  // reset clocks
  clocks_init();
  // stdio_init_all();

  return;
}

static void sleep_callback(void)
{
  critical_section_enter_blocking(&sleep_crit_sec);
  sleep_alarm = true;
  critical_section_exit(&sleep_crit_sec);

  return;
}

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

// will sleep to max 23hours, 59 mins and 59 seconds
static void rtc_sleep_seconds(int8_t seconds_to_sleep)
{
  int8_t hours;
  int8_t minutes;
  int8_t seconds;

  // crudely reset the clock each time
  // to the value below
  datetime_t dt = {
      .year = 1977,
      .month = 1,
      .day = 1,
      .dotw = 0, // 0 is Sunday
      .hour = 0,
      .min = 0,
      .sec = 0};

  sleep_run_from_xosc();
  // Reset real time clock to a known value
  rtc_set_datetime(&dt);

  hours = round(seconds_to_sleep / 3600);
  if (hours >= 24)
  {
    hours = 23;
    seconds_to_sleep = (hours * 3600) + (59 * 60) + 59;
  }
  minutes = round((seconds_to_sleep - (hours * 3600)) / 60);
  seconds = round(seconds_to_sleep - (hours * 3600) - (minutes * 60));
  dt.hour = hours;
  dt.min = minutes;
  dt.sec = seconds;

  sleep_goto_sleep_until(&dt, &sleep_callback);
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

    if ((now - wind_last_ts_usec) >= wind_bounce_delta_usec)
    {
      wind_last_ts_usec = now;
      critical_section_enter_blocking(&wind_crit_sec);
      wind_irq++;
      critical_section_exit(&wind_crit_sec);
    }
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

int main()
{
  // for saving clock values
  uint scb_orig;
  uint clock0_orig;
  uint clock1_orig;

  // stdio_init_all();

  set_low_power();

  setup_i2c_slave(I2C_SLAVE_ADDRESS, I2C_SLAVE_SDA_PIN, I2C_SLAVE_SCL_PIN);

  auto ledStrip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, LED_PIN, LED_LENGTH, PicoLed::FORMAT_GRB);
  signal_startup_with_leds(ledStrip);

  /* Critical sections */
  critical_section_init(&sleep_crit_sec);
  critical_section_init(&bucket_crit_sec);
  critical_section_init(&wind_crit_sec);

  init_gpios();

  // Start the Real time clock
  rtc_init();

  while (true)
  {
    // save clock values for later
    scb_orig = scb_hw->scr;
    clock0_orig = clocks_hw->sleep_en0;
    clock1_orig = clocks_hw->sleep_en1;

    // static bool gpio_get	(	uint 	gpio	)
    // inlinestatic
    // Get state of a single specified GPIO.

    // Parameters
    // gpio	GPIO number
    // Returns
    // Current state of the GPIO. 0 for low, non-zero for high

#if 0
    // sleep here, in this case for 10 seconds
    rtc_sleep_seconds(10);

    // reset processor and clocks back to defaults
    recover_from_sleep(scb_orig, clock0_orig, clock1_orig);
#endif

    if (bucket_irq || wind_irq)
    {
      critical_section_enter_blocking(&bucket_crit_sec);
      bucket_irq = 0;
      critical_section_exit(&bucket_crit_sec);
      critical_section_enter_blocking(&wind_crit_sec);
      wind_irq = 0;
      critical_section_exit(&wind_crit_sec);

      blink_led(ledStrip, PicoLed::RGB(0, 255, 255), 25);
    }

    if (sleep_alarm)
    {
      critical_section_enter_blocking(&sleep_crit_sec);
      sleep_alarm = false;
      critical_section_exit(&sleep_crit_sec);
      blink_led(ledStrip, PicoLed::RGB(255, 255, 0), 25);
    }

    __wfi();
  }
}