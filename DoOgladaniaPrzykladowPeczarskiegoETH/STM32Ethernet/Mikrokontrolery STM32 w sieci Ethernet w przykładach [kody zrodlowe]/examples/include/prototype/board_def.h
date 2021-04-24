#ifndef _BOARD_DEF_H
#define _BOARD_DEF_H 1

#include <board_defs.h>

#define BOARD_TYPE PROTOTYPE
#define ETH_BOARD  STE100P

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
#if HSE_VALUE != 25000000
  #error PROTOTYPE board uses 25 MHz external quarz.
#endif

#endif
