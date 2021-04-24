#ifndef _UTIL_WDG_H
#define _UTIL_WDG_H 1

#include <stm32f10x_iwdg.h>

#define WatchdogReset IWDG_ReloadCounter

void WatchdogStart(unsigned);

#endif
