#include <board_lcd.h>
#include <util_lcd.h>

/* Te funkcje nie są współużywalne. */

void LCDwrite(const char *s) {
  while (*s)
    LCDputchar(*s++);
}

void LCDwriteWrap(const char *s) {
  while (*s)
    LCDputcharWrap(*s++);
}

void LCDwriteLen(const char *s, unsigned n) {
  while (n-- > 0)
    LCDputchar(*s++);
}

void LCDwriteLenWrap(const char *s, unsigned n) {
  while (n-- > 0)
    LCDputcharWrap(*s++);
}
