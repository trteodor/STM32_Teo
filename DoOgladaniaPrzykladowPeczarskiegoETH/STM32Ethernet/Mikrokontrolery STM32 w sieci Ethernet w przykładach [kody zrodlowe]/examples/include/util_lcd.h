#ifndef _UTIL_LCD_H
#define _UTIL_LCD_H 1

void LCDwrite(const char *);
void LCDwriteWrap(const char *);
void LCDwriteLen(const char *, unsigned);
void LCDwriteLenWrap(const char *, unsigned);

#endif
