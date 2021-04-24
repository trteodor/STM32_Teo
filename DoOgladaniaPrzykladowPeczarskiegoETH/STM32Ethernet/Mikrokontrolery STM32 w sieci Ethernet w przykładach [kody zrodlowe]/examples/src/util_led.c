#include <board_led.h>
#include <util_delay.h>
#include <util_led.h>

static const int delayTime = 1800000;

void OK(int n) {
  int i;

  GreenLEDoff();
  for (i = 0; i < n; ++i) {
    Delay(delayTime);
    GreenLEDon();
    Delay(delayTime);
    GreenLEDoff();
  }
  Delay(9 * delayTime);
}

void Error(int n) {
  int i;

  RedLEDoff();
  for (i = 0; i < n; ++i) {
    Delay(delayTime);
    RedLEDon();
    Delay(delayTime);
    RedLEDoff();
  }
  Delay(9 * delayTime);
}
