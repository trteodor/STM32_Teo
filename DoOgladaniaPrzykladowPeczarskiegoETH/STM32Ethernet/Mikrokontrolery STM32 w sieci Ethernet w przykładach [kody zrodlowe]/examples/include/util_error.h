#ifndef _UTIL_ERROR_H
#define _UTIL_ERROR_H 1

#include <stm32f10x.h>
#include <util_led.h>
#include <util_rtc.h>

/* Obługa błędów, których przyczyna wydaje się być nieusuwalna. */
#define error_permanent(expr, err) error_check(expr, err)

/* Obługa błędów, których przyczyna może ustąpić po resecie. */
#define error_resetable(expr, err)      \
  if ((expr) < 0) {                     \
    int i;                              \
    for (i = 0; i < 3; ++i) {           \
      Error(err);                       \
    }                                   \
    NVIC_SystemReset();                 \
  }

/* Obługa błędów, których przyczyna może ustąpić po czasie. */
#define error_temporary(expr, err)      \
  if ((expr) < 0) {                     \
    int i;                              \
    for (i = 0; i < 3; ++i) {           \
      Error(err);                       \
    }                                   \
    Standby(10);                        \
  }

#endif
