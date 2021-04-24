#ifndef _UTIL_TIME_H
#define _UTIL_TIME_H 1

#define SYSTICK_FREQUENCY 100
#define SYSTICK_PERIOD_MS (1000 / SYSTICK_FREQUENCY)

typedef unsigned long long ltime_t;
typedef void (*time_callback_t)(void);

int LocalTimeConfigure(void);
void TimerCallBack(void (*)(void));
volatile ltime_t LocalTime(void);
void GetLocalTime(unsigned *,
                  unsigned *,
                  unsigned *,
                  unsigned *,
                  unsigned *);

#endif
