#ifndef _UTIL_RTC_H
#define _UTIL_RTC_H 1

#include <stdint.h>

int RTCconfigure(void);
void RTCsetCallback(void (*)(void));
uint32_t GetRealTimeClock(void);
void SetRealTimeClock(uint32_t);
void Standby(uint32_t);
void DelayedStandby(uint32_t, uint32_t);

#endif
