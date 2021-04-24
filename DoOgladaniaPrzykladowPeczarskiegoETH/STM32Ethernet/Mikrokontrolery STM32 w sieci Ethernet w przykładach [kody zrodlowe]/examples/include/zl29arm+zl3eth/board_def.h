#ifndef _BOARD_DEF_H
#define _BOARD_DEF_H 1

#include <board_defs.h>

#define BOARD_TYPE ZL29ARM
#define ETH_BOARD  ZL3ETH

/* Wszystkie stałe powinny być definiowane w plikach nagłówkowych,
   ale Standard Peripherals Library na to nie pozwala... */
#ifndef USE_STDPERIPH_DRIVER
  #error USE_STDPERIPH_DRIVER not defined
#endif
#ifndef STM32F10X_CL
  #error STM32F10X_CL not defined
#endif
#ifndef HSE_VALUE
  #error HSE_VALUE not defined
#endif

/* Zapewnij spójność stałych definiowanych w różnych miejscach. */
#if HSE_VALUE != 10000000
  #error ZL29ARM board uses 10 MHz external quarz.
#endif

#endif
