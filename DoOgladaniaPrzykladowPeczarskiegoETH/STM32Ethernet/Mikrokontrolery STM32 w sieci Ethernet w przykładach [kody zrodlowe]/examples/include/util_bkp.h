#ifndef _UTIL_BKP_H
#define _UTIL_BKP_H 1

#include <stdint.h>

void BKPconfigure(void);
int BKPnumberWords(void);
int BKPwrite(const uint32_t *, int);
int BKPread(uint32_t *, int);

#endif
