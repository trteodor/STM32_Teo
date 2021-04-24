#ifndef _UTIL_LED_H
#define _UTIL_LED_H 1

void OK(int);
void Error(int);

#define error_check(expr, err)  \
  if ((expr) < 0) {             \
    for (;;) {                  \
      Error(err);               \
    }                           \
  }

#endif
