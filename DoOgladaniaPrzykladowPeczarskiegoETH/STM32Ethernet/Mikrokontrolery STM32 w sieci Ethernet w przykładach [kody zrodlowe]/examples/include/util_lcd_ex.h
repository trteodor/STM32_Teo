#ifndef _UTIL_LCD_EX_H
#define _UTIL_LCD_EX_H 1

#include <time.h>
#include <lwip/ip_addr.h>

void LCDwriteIP(const struct ip_addr *);
void LCDwriteIPport(const struct ip_addr *, uint16_t);
void LCDwriteRXTX(unsigned, unsigned, unsigned, unsigned);
void LCDwriteTime(time_t);

#endif
