#include <util_delay.h>

void Delay(volatile unsigned count) {
  while(count--);
}
