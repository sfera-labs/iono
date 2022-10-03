/*
  Watchdog.h

    Copyright (C) 2018-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef Watchdog_h
#define Watchdog_h

class Watchdog {
  private:
    static unsigned long _ts;

  public:
    static void setup();
    static void disable();
    static void clear();
};

unsigned long  Watchdog::_ts;

void Watchdog::disable() {
  REG_WDT_CTRL &= ~WDT_CTRL_ENABLE;
  while(WDT->STATUS.bit.SYNCBUSY);
}

void Watchdog::setup() {
  // Set up the generic clock (GCLK2) used to clock the watchdog timer at 1.024kHz
  REG_GCLK_GENDIV = GCLK_GENDIV_DIV(4) |            // Divide the 32.768kHz clock source by divisor 32, where 2^(4 + 1): 32.768kHz/32=1.024kHz
                    GCLK_GENDIV_ID(2);              // Select Generic Clock (GCLK) 2
  while (GCLK->STATUS.bit.SYNCBUSY);                // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_DIVSEL |          // Set to divide by 2^(GCLK_GENDIV_DIV(4) + 1)
                     GCLK_GENCTRL_IDC |             // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |           // Enable GCLK2
                     GCLK_GENCTRL_SRC_OSCULP32K |   // Set the clock source to the ultra low power oscillator (OSCULP32K)
                     GCLK_GENCTRL_ID(2);            // Select GCLK2
  while (GCLK->STATUS.bit.SYNCBUSY);                // Wait for synchronization

  // Feed GCLK2 to WDT (Watchdog Timer)
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |           // Enable GCLK2 to the WDT
                     GCLK_CLKCTRL_GEN_GCLK2 |       // Select GCLK2
                     GCLK_CLKCTRL_ID_WDT;           // Feed the GCLK2 to the WDT
  while (GCLK->STATUS.bit.SYNCBUSY);                // Wait for synchronization

  REG_WDT_CONFIG = WDT_CONFIG_PER_8K;               // Set the WDT reset timeout to about 8 second
  while(WDT->STATUS.bit.SYNCBUSY);                  // Wait for synchronization
  REG_WDT_CTRL = WDT_CTRL_ENABLE;                   // Enable the WDT in normal mode
  while(WDT->STATUS.bit.SYNCBUSY);                  // Wait for synchronization

  _ts = millis();
}

void Watchdog::clear() {
  if (!WDT->STATUS.bit.SYNCBUSY && millis() - _ts >= 300) {
    REG_WDT_CLEAR = WDT_CLEAR_CLEAR_KEY;
    _ts = millis();
  }
}

extern Watchdog Watchdog;

#endif
