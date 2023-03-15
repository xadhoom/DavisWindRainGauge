#include <pico/stdlib.h>
#include <hardware/pll.h>
#include <hardware/clocks.h>
#include <hardware/xosc.h>
#include <hardware/rosc.h>
#include <hardware/uart.h>
// For scb_hw so we can enable deep sleep
#include <hardware/structs/scb.h>
#include "low_power.h"

extern void set_low_power()
{
  clocks_init();

  // Change clk_sys to be 48MHz. The simplest way is to take this from PLL_USB
  // which has a source frequency of 48MHz
  clock_configure(clk_sys,
                  CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                  CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                  48 * MHZ,
                  12 * MHZ);

  // Turn off PLL sys for good measure
  pll_deinit(pll_sys);

  clock_configure(clk_adc,
                  0, // No GLMUX
                  CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                  48 * MHZ,
                  48 * MHZ);

  // CLK peri is clocked from clk_sys so need to change clk_peri's freq
  clock_configure(clk_peri,
                  0,
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  48 * MHZ,
                  12 * MHZ);

  rosc_disable();

  clock_stop(clk_peri);
}

extern void set_low_power_b()
{
  uint src_hz = XOSC_MHZ * MHZ;

  clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                  0, // No aux mux
                  src_hz, src_hz);

  // CLK SYS = CLK_REF
  clock_configure(clk_sys,
                  CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                  0, // Using glitchless mux
                  src_hz,
                  src_hz * 4);

  clock_configure(clk_rtc,
                  0, // No GLMUX
                  CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC,
                  src_hz,
                  46875);

  // // CLK PERI = clk_sys. Used as reference clock for Peripherals. No dividers so just select and enable
  clock_configure(clk_peri,
                  0,
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  src_hz,
                  src_hz);

  rosc_disable();
  // xosc_disable();

  clock_stop(clk_usb);
  clock_stop(clk_peri);

  pll_deinit(pll_usb);

  uart_deinit(uart0);
  uart_deinit(uart1);

  // Turn off not needed clocks
  // uint clocks0 = clocks_hw->sleep_en0;
  uint bits0 = CLOCKS_SLEEP_EN0_BITS ^ CLOCKS_SLEEP_EN0_CLK_SYS_ROSC_BITS ^ CLOCKS_SLEEP_EN0_CLK_SYS_SPI0_BITS ^ CLOCKS_SLEEP_EN0_CLK_PERI_SPI0_BITS ^ CLOCKS_SLEEP_EN0_CLK_SYS_SPI1_BITS ^ CLOCKS_SLEEP_EN0_CLK_PERI_SPI1_BITS ^ CLOCKS_SLEEP_EN0_CLK_SYS_PLL_USB_BITS ^
               CLOCKS_SLEEP_EN0_CLK_SYS_JTAG_BITS ^ CLOCKS_SLEEP_EN0_CLK_SYS_I2C1_BITS;
  uint bits1 = CLOCKS_SLEEP_EN1_CLK_SYS_XOSC_BITS;
  // clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_BITS;
  // clocks_hw->sleep_en1 = 0x0;
  // // Enable deep sleep at the proc
  // uint save = scb_hw->scr;
  // scb_hw->scr = save | M0PLUS_SCR_SLEEPDEEP_BITS;

  uint bits_wake0 = CLOCKS_WAKE_EN0_BITS ^ CLOCKS_WAKE_EN0_CLK_SYS_I2C1_BITS ^ CLOCKS_WAKE_EN0_CLK_SYS_JTAG_BITS ^ CLOCKS_WAKE_EN0_CLK_SYS_PLL_USB_BITS ^ CLOCKS_WAKE_EN0_CLK_SYS_ROSC_BITS ^ CLOCKS_WAKE_EN0_CLK_PERI_SPI0_BITS ^ CLOCKS_WAKE_EN0_CLK_SYS_SPI0_BITS ^ CLOCKS_WAKE_EN0_CLK_PERI_SPI1_BITS ^ CLOCKS_WAKE_EN0_CLK_SYS_SPI1_BITS;
  uint bits_wake1 = CLOCKS_WAKE_EN1_CLK_SYS_TIMER_BITS | CLOCKS_WAKE_EN1_CLK_SYS_XIP_BITS | CLOCKS_WAKE_EN1_CLK_SYS_XOSC_BITS | CLOCKS_WAKE_EN1_CLK_SYS_SRAM4_BITS | CLOCKS_WAKE_EN1_CLK_SYS_SRAM5_BITS;
  // clocks_hw->wake_en0 = bits_wake0;
  // clocks_hw->wake_en1 = bits_wake1;
  // clocks_init();
}