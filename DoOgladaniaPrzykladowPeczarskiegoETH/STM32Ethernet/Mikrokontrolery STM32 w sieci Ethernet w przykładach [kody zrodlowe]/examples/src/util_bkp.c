#include <stm32f10x_bkp.h>
#include <stm32f10x_crc.h>
#include <stm32f10x_pwr.h>
#include <stm32f10x_rcc.h>
#include <util_bkp.h>

#ifdef STM32F10X_CL
  static const uint16_t BKPdataReg[] = {
    BKP_DR1,  BKP_DR2,  BKP_DR3,  BKP_DR4,  BKP_DR5,  BKP_DR6,
    BKP_DR7,  BKP_DR8,  BKP_DR9,  BKP_DR10, BKP_DR11, BKP_DR12,
    BKP_DR13, BKP_DR14, BKP_DR15, BKP_DR16, BKP_DR17, BKP_DR18,
    BKP_DR19, BKP_DR20, BKP_DR21, BKP_DR22, BKP_DR23, BKP_DR24,
    BKP_DR25, BKP_DR26, BKP_DR27, BKP_DR28, BKP_DR29, BKP_DR30,
    BKP_DR31, BKP_DR32, BKP_DR33, BKP_DR34, BKP_DR35, BKP_DR36,
    BKP_DR37, BKP_DR38, BKP_DR39, BKP_DR40, BKP_DR41, BKP_DR42
  };
#else
  static const uint16_t BKPdataReg[] = {
    BKP_DR1,  BKP_DR2,  BKP_DR3,  BKP_DR4,  BKP_DR5,
    BKP_DR6,  BKP_DR7,  BKP_DR8,  BKP_DR9,  BKP_DR10
  };
#endif

/* Liczba 32-bitowych słów, które można zapamiętać.
   Jedno słowo rezerwuj na CRC. */
static const int BKPwords =
  (sizeof(BKPdataReg) / (2 * sizeof(BKPdataReg[0]))) - 1;

void BKPconfigure() {
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,
                         ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
  PWR_BackupAccessCmd(ENABLE);

  /* Przycisk podłączony do PC13 służy do kasowania zawartości. */
  BKP_ClearFlag();
  BKP_TamperPinLevelConfig(BKP_TamperPinLevel_Low);
  BKP_TamperPinCmd(ENABLE);
}

int BKPnumberWords(void) {
  return BKPwords;
}

int BKPwrite(const uint32_t *data, int count) {
  int idx;
  uint32_t crc;

  if (count > BKPwords)
    count = BKPwords;
  CRC_ResetDR();
  for (idx = 0; idx < count; ++idx) {
    BKP_WriteBackupRegister(BKPdataReg[2 * idx],
                            data[idx] & 0xffff);
    BKP_WriteBackupRegister(BKPdataReg[2 * idx + 1],
                            data[idx] >> 16);
    CRC_CalcCRC(data[idx]);
  }
  crc = CRC_GetCRC();
  BKP_WriteBackupRegister(BKPdataReg[2 * idx], crc & 0xffff);
  BKP_WriteBackupRegister(BKPdataReg[2 * idx + 1], crc >> 16);
  return idx;
}

int BKPread(uint32_t *data, int count) {
  int idx;
  uint32_t crc;

  if (count > BKPwords)
    count = BKPwords;
  for (idx = 0; idx < count; ++idx)
    data[idx] = BKP_ReadBackupRegister(BKPdataReg[2 * idx]) |
      (BKP_ReadBackupRegister(BKPdataReg[2 * idx + 1]) << 16);
  crc = BKP_ReadBackupRegister(BKPdataReg[2 * idx]) |
        (BKP_ReadBackupRegister(BKPdataReg[2 * idx + 1]) << 16);
  CRC_ResetDR();
  if (crc != CRC_CalcBlockCRC(data, idx))
    return -1;
  return idx;
}
