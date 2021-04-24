#include <util_wdg.h>

static const uint8_t prescalerTbl[] = {
  IWDG_Prescaler_4,
  IWDG_Prescaler_8,
  IWDG_Prescaler_16,
  IWDG_Prescaler_32,
  IWDG_Prescaler_64,
  IWDG_Prescaler_128,
  IWDG_Prescaler_256
};
static const int maxIdx =
  sizeof(prescalerTbl) / sizeof(prescalerTbl[0]) - 1;

/* IWDG jest taktowany zegarem LSI. Częstotliwość LSI ma duży rozrzut
   od 30 do 60 kHz. Do obliczeń zakładamy wartość nominalną 40 kHz.
   Wartość parametru timeout jest podawana w ms.
   Uaktywnienie IWDG powoduje włączenie LSI. */
void WatchdogStart(unsigned timeout) {
  int idx;

  timeout *= 10U;
  for (idx = 0;
       timeout > 0x1000U && idx <= maxIdx;
       timeout >>= 1, ++idx);
  if (idx > maxIdx) {
    idx = maxIdx;
    timeout = 0x1000U;
  }
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(prescalerTbl[idx]);
  IWDG_SetReload(timeout - 1);
  IWDG_ReloadCounter();
  IWDG_Enable();
}
