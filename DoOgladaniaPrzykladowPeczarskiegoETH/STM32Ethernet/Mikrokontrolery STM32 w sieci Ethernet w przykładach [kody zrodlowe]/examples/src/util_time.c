#include <arch/cc.h>
#include <util_time.h>
#include <stm32f10x_rcc.h>

static volatile ltime_t localTime = 0;
static time_callback_t timeCallBack = 0;

void SysTick_Handler() {
  localTime += SYSTICK_PERIOD_MS;
  if (timeCallBack)
    timeCallBack();
}

int LocalTimeConfigure() {
  RCC_ClocksTypeDef RCC_Clocks;

  RCC_GetClocksFreq(&RCC_Clocks);
  if (SysTick_Config(RCC_Clocks.HCLK_Frequency / SYSTICK_FREQUENCY))
    return -1;
  /* SysTick_Config ustawia jakiś priorytet, trzeba go zmienić. */
  SET_PRIORITY(SysTick_IRQn, HIGH_IRQ_PRIO, 0);
  return 0;
}

void TimerCallBack(time_callback_t f) {
  IRQ_DECL_PROTECT(x);

  IRQ_PROTECT(x, HIGH_IRQ_PRIO);
  timeCallBack = f;
  IRQ_UNPROTECT(x);
}

volatile ltime_t LocalTime() {
  ltime_t t;
  IRQ_DECL_PROTECT(x);

  IRQ_PROTECT(x, HIGH_IRQ_PRIO);
  t = localTime;
  IRQ_UNPROTECT(x);
  return t;
}

void GetLocalTime(unsigned *day,
                  unsigned *hour,
                  unsigned *minute,
                  unsigned *second,
                  unsigned *milisecond) {
  ltime_t t = LocalTime();
  *milisecond = t % 1000;
  t /= 1000;
  *second = t % 60;
  t /= 60;
  *minute = t % 60;
  t /= 60;
  *hour = t % 24;
  t /= 24;
  *day = t;
}
