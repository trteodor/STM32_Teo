#ifndef _BOARD_INIT_H
#define _BOARD_INIT_H 1

#include <stm32f10x_gpio.h>

void AllPinsDisable(void);
int CLKconfigure(void);
int ETHconfigureMII(void);
void ETHpowerDown(void);
void ETHpowerUp(void);

#define active_check(cond, limit) {     \
  int i;                                \
  for (i = (limit); !(cond); --i)       \
    if (i <= 0)                         \
      return -1;                        \
}

extern GPIO_TypeDef * const gpio[];
extern const int gpioCount;

#endif
