#ifndef _STM32_ARCH_H
#define _STM32_ARCH_H 1

/* Zamiast stm32f10x.h powinno w zasadzie wystarczyć włączenie
   core_cm3.h, ale wtedy nie kompiluje się... */
#include <stm32f10x.h>
#include <stdlib.h>

#define BYTE_ORDER LITTLE_ENDIAN

/* Stos lwIP używa własnych nazw dla typów całościowych (ang.
   integral types). Ogólny wskaźnik mem_ptr_t nie jest zdefiniowany
   jako void*, aby możliwe były wszystkie operacje arytmetyczne. */
typedef int8_t   s8_t;
typedef uint8_t  u8_t;
typedef int16_t  s16_t;
typedef uint16_t u16_t;
typedef int32_t  s32_t;
typedef uint32_t u32_t;
typedef u32_t    mem_ptr_t;

/* Stos lwIP potrzebuje specyfikatorów formatowania dla funkcji
   *printf. Typy int16_t, uint16_t, int32_t, uint32_t są zdefiniowane
   w stdint.h odpowiednio jako short, unsigned short, long,
   unsigned long. SZ_T specyfikuje format dla typu size_t. */
#define S16_F "hd"
#define U16_F "hu"
#define X16_F "hx"
#define S32_F "ld"
#define U32_F "lu"
#define X32_F "lx"
#define SZT_F "zu"

#if defined __GNUC__
  #define ALIGN4 __attribute__ ((aligned (4)))
#else
  #error Define ALIGN4 macro for your compiler.
#endif

/* Optymalizuj operacje odwracania kolejności bajtów - bardzo
   przydatne w architekturze little-endian. */
#if defined __GNUC__
  #define LWIP_PLATFORM_HTONS(x) ({                 \
    u16_t result;                                   \
    asm ("rev16 %0, %1" : "=r" (result) : "r" (x)); \
    result;                                         \
  })
  #define LWIP_PLATFORM_HTONL(x) ({                 \
    u32_t result;                                   \
    asm ("rev %0, %1" : "=r" (result) : "r" (x));   \
    result;                                         \
  })
#else
  #define LWIP_PLATFORM_HTONS(x) __REV16(x)
  #define LWIP_PLATFORM_HTONL(x) __REV(x)
#endif

/* Optymalizuj czas działania - nie używaj asercji. */
#define LWIP_PLATFORM_DIAG(x) ((void)0)
#define LWIP_PLATFORM_ASSERT(x) ((void)0)
/* Oryginalnie było tak
#define LWIP_PLATFORM_ASSERT(x) do { if(!(x)) while(1); } while(0)
*/

/* Jeśli w lwiopts.h zdefiniowano opcję SYS_LIGHTWEIGHT_PROT == 1,
   trzeba też zdefiniować makra chroniące dostępy do pamięci. Makra
   te pojawiają się w funkcjach memp_free, memp_malloc, pbuf_ref,
   pbuf_free oraz gdy zdefiniowano MEM_LIBC_MALLOC == 0, to również
   w mem_realloc i mem_free, choć wtedy między wystąpieniami
   SYS_ARCH_PROTECT i SYS_ARCH_UNPROTECT nie ma żadnego kodu. */
#if defined __GNUC__
  #define SYS_ARCH_DECL_PROTECT(x)                \
    u32_t x
  #define SYS_ARCH_PROTECT(x) ({                  \
    asm volatile (                                \
      "mrs %0, PRIMASK\n\t"                       \
      "cpsid i" :                                 \
      "=r" (x) :                                  \
    );                                            \
  })
  #define SYS_ARCH_UNPROTECT(x) ({                \
    asm volatile ("msr PRIMASK, %0" : : "r" (x)); \
  })
#else
  #define SYS_ARCH_DECL_PROTECT(x)                \
    u32_t x
  #define SYS_ARCH_PROTECT(x)                     \
    (x = __get_PRIMASK(), __disable_irq())
  #define SYS_ARCH_UNPROTECT(x)                   \
    __set_PRIMASK(x)
#endif

/* Definiujemy trzy priorytety przerwań. */
#define HIGH_IRQ_PRIO 1
#define LWIP_IRQ_PRIO 2
#define LOW_IRQ_PRIO  3
#define PREEMPTION_PRIORITY_BITS 2

/* Uaktywnij możliwość blokowania przerwań zależnie od ich
   priorytetu. */
#define SET_IRQ_PROTECTION()                            \
  NVIC_SetPriorityGrouping(7 - PREEMPTION_PRIORITY_BITS)

/* Definiujemy własne makro, gdyż funkcja NVIC_SetPriority
   z biblioteki CMSIS nie uwzględnia zmienionej liczby bitów
   priorytetu i podpriorytetu. */
#define SET_PRIORITY(irq, prio, subprio)           \
  NVIC_SetPriority((irq), (subprio) | ((prio) <<   \
    (__NVIC_PRIO_BITS - PREEMPTION_PRIORITY_BITS)))

/* Makro IRQ_DECL_PROTECT(prv) deklaruje zmienną prv, która
   przechowuje informację o blokowanych przerwaniach. Makro
   IRQ_PROTECT(prv, lvl) zapisuje w zmiennej prv stan aktualnie
   blokowanych przerwań i blokuje przerwania o priorytecie niższym
   lub równym lvl. Makro IRQ_UNPROTECT(prv) przywraca stan blokowania
   przerwań zapisany w zmiennej prv. */
#if defined __GNUC__
  #define IRQ_DECL_PROTECT(prv)                             \
    u32_t prv
  #define IRQ_PROTECT(prv, lvl) ({                          \
    u32_t tmp;                                              \
    asm volatile (                                          \
      "mrs %0, BASEPRI\n\t"                                 \
      "movs %1, %2\n\t"                                     \
      "msr BASEPRI, %1" :                                   \
      "=r" (prv), "=r" (tmp) :                              \
      "i" ((lvl) << (8 - PREEMPTION_PRIORITY_BITS))         \
    );                                                      \
  })
  #define IRQ_UNPROTECT(prv) ({                             \
    asm volatile ("msr BASEPRI, %0" : : "r" (prv));         \
  })
#else
  #define IRQ_DECL_PROTECT(prv)                             \
    u32_t prv
  #define IRQ_PROTECT(prv, lvl)                             \
    (prv = __get_BASEPRI(),                                 \
     __set_BASEPRI((lvl) << (8 - PREEMPTION_PRIORITY_BITS)))
  #define IRQ_UNPROTECT(prv)                                \
    __set_BASEPRI(prv)
#endif

/* W pliku lwiopts.h nazwy funkcji obsługujących alokację pamięci
   mem_* są przedefiniowane na protected_*. Tu są ich definicje. */

static __INLINE void protected_free(void *ptr) {
  SYS_ARCH_DECL_PROTECT(v);
  SYS_ARCH_PROTECT(v);
  free(ptr);
  SYS_ARCH_UNPROTECT(v);
}

static __INLINE void *protected_calloc(size_t nmemb, size_t size) {
  void *ret;
  SYS_ARCH_DECL_PROTECT(v);
  SYS_ARCH_PROTECT(v);
  ret = calloc(nmemb, size);
  SYS_ARCH_UNPROTECT(v);
  return ret;
}

static __INLINE void *protected_malloc(size_t size) {
  void *ret;
  SYS_ARCH_DECL_PROTECT(v);
  SYS_ARCH_PROTECT(v);
  ret = malloc(size);
  SYS_ARCH_UNPROTECT(v);
  return ret;
}

static __INLINE void *protected_realloc(void *ptr, size_t size) {
  void *ret;
  SYS_ARCH_DECL_PROTECT(v);
  SYS_ARCH_PROTECT(v);
  ret = realloc(ptr, size);
  SYS_ARCH_UNPROTECT(v);
  return ret;
}

#endif
