#include <board_led.h>
#include <util_delay.h>

int main(void) {
  static const unsigned delay_time = 2000000;

  LEDconfigure();
  for (;;) {
    RedLEDon();
    Delay(delay_time);
    GreenLEDon();
    Delay(delay_time);
    RedLEDoff();
    Delay(delay_time);
    GreenLEDoff();
    Delay(delay_time);
  }
}
