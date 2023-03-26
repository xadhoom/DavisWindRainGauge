#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/pll.h>
#include <hardware/clocks.h>
#include <hardware/xosc.h>
#include <hardware/rosc.h>
#include <hardware/uart.h>
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
                  12 * MHZ);

  // CLK peri is clocked from clk_sys so need to change clk_peri's freq
  clock_configure(clk_peri,
                  0,
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  12 * MHZ,
                  12 * MHZ);

  rosc_disable();

  clock_stop(clk_peri);
}
