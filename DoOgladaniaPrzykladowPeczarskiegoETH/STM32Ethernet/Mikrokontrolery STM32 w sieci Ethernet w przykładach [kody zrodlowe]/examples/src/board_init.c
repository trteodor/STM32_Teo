#include <board_def.h>
#include <board_init.h>
#include <stm32_eth.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_rcc.h>

#if HSE_VALUE == 25000000
  #define RCC_PREDIV2_DivX RCC_PREDIV2_Div5
#elif HSE_VALUE == 20000000
  #define RCC_PREDIV2_DivX RCC_PREDIV2_Div4
#elif HSE_VALUE == 15000000
  #define RCC_PREDIV2_DivX RCC_PREDIV2_Div3
#elif HSE_VALUE == 10000000
  #define RCC_PREDIV2_DivX RCC_PREDIV2_Div2
#elif HSE_VALUE == 5000000
  #define RCC_PREDIV2_DivX RCC_PREDIV2_Div1
#else
  #error Wrong HSE_VALUE
#endif

#define ETH_NO            0
#define ETH_MII           1
#define ETH_MII_REMAPED   2
#define ETH_RMII          3
#define ETH_RMII_REMAPED  4

#ifndef BOARD_TYPE
  #error BOARD_TYPE undefined
#endif

#if BOARD_TYPE == PROTOTYPE || \
    BOARD_TYPE == BUTTERFLY || \
    BOARD_TYPE == ZL29ARM
  /* Tablica widoczna na zewnątrz tej jednostki translacji */
  GPIO_TypeDef * const gpio[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
  const int gpioCount = sizeof(gpio) / sizeof(gpio[0]);
  /* Zmienne niewidoczne na zewnątrz tej jednostki translacji */
  static const uint32_t allRccGpio = RCC_APB2Periph_GPIOA |
                                     RCC_APB2Periph_GPIOB |
                                     RCC_APB2Periph_GPIOC |
                                     RCC_APB2Periph_GPIOD |
                                     RCC_APB2Periph_GPIOE;
  static const int ethInterface = ETH_MII_REMAPED;
#else
  #error Undefined BOARD_TYPE
#endif

#if BOARD_TYPE == PROTOTYPE || BOARD_TYPE == BUTTERFLY
  /* Wyprowadzenie PWRDWN modułu ethernetowego jest podłączone do
     PD13 mikrokontrolera. */
  static GPIO_TypeDef * const gpioPowerDown = GPIOD;
  static const uint32_t rccPowerDown = RCC_APB2Periph_GPIOD;
  static const uint16_t gpioPowerDownPin = GPIO_Pin_13;
#elif BOARD_TYPE == ZL29ARM
  /* Wyprowadzenie PWRDWN modułu ethernetowego jest podłączone do
     PC0 mikrokontrolera. */
  static GPIO_TypeDef * const gpioPowerDown = GPIOC;
  static const uint32_t rccPowerDown = RCC_APB2Periph_GPIOC;
  static const uint16_t gpioPowerDownPin = GPIO_Pin_0;
#endif

/* Konfiguruj wszystkie wyprowadzenia w analogowym trybie wejściowym
   (ang. analog input mode, trigger off), co redukuje zużycie energii
   i zwiększa odporność na zakłócenia elektromagnetyczne. */
void AllPinsDisable() {
  GPIO_InitTypeDef GPIO_InitStructure;
  int i;

  RCC_APB2PeriphClockCmd(allRccGpio, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = 0;
  for (i = 0; i < sizeof(gpio) / sizeof(gpio[0]); ++i)
    GPIO_Init(gpio[i], &GPIO_InitStructure);
  RCC_APB2PeriphClockCmd(allRccGpio, DISABLE);
}

static void ETHinterfaceMIIconfigure(void);
static void ETHinterfaceRemapedMIIconfigure(void);
static void ETHinterfaceRMIIconfigure(void);
static void ETHinterfaceRemapedRMIIconfigure(void);
static void ETHpowerConfigure(void);

int CLKconfigure(void) {
  static const int maxAttempts = 1000000;

  /* Wołanie SystemInit() jest niepotrzebne. */
  RCC_DeInit();
  RCC_HSEConfig(RCC_HSE_ON);
  /* Wykonuje maksymalnie HSEStartUp_TimeOut = 0x0500 prób. */
  if (RCC_WaitForHSEStartUp() == ERROR)
    return -1;

  FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
  FLASH_SetLatency(FLASH_Latency_2);

  /* preskaler AHB, HCLK = SYSCLK = 72 MHz */
  RCC_HCLKConfig(RCC_SYSCLK_Div1);

  /* preskaler APB1, PCLK1 = HCLK / 2 = 36 MHz */
  RCC_PCLK1Config(RCC_HCLK_Div2);

  /* preskaler APB2, PCLK2 = HCLK = 72 MHz */
  RCC_PCLK2Config(RCC_HCLK_Div1);

  /* PLL2: PLL2CLK = HSE / RCC_PREDIV2_DivX * 8 = 40 MHz */
  RCC_PREDIV2Config(RCC_PREDIV2_DivX);
  RCC_PLL2Config(RCC_PLL2Mul_8);
  RCC_PLL2Cmd(ENABLE);
  active_check(RCC_GetFlagStatus(RCC_FLAG_PLL2RDY), maxAttempts);

  if (ethInterface != ETH_NO) {
    /* PLL3: PLL3CLK = (HSE / 5) * 10 = 50 MHz */
    RCC_PLL3Config(RCC_PLL3Mul_10);
    RCC_PLL3Cmd(ENABLE);
    active_check(RCC_GetFlagStatus(RCC_FLAG_PLL3RDY), maxAttempts);
  }

  /* PLL1: PLLCLK = (PLL2 / 5) * 9 = 72 MHz */
  RCC_PREDIV1Config(RCC_PREDIV1_Source_PLL2, RCC_PREDIV1_Div5);
  RCC_PLLConfig(RCC_PLLSource_PREDIV1, RCC_PLLMul_9);
  RCC_PLLCmd(ENABLE);
  active_check(RCC_GetFlagStatus(RCC_FLAG_PLLRDY), maxAttempts);

  /* Ustaw SYSCLK = PLLCLK i czekaj aż PLL zostanie ustawiony jako
     zegar systemowy. */
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
  active_check(RCC_GetSYSCLKSource() == 0x08, maxAttempts);

  if (ethInterface == ETH_MII || ethInterface == ETH_MII_REMAPED)
    /* MCO (PA8) = PLL3CLK / 2 = 25 MHz */
    RCC_MCOConfig(RCC_MCO_PLL3CLK_Div2);
    /* Jeśli mamy kwarc 25 MHz (HSE_VALUE == 25000000),
       można ominąć PLL3...
    RCC_MCOConfig(RCC_MCO_XT1); */
  else if (ethInterface == ETH_RMII ||
           ethInterface == ETH_RMII_REMAPED)
    /* MCO (PA8) = PLL3CLK = 50 MHz */
    RCC_MCOConfig(RCC_MCO_PLL3CLK);

  return 0;
}

int ETHconfigureMII(void) {
  static const int maxAttempts = 1000000;

  if (ethInterface == ETH_MII)
    ETHinterfaceMIIconfigure();
  else if (ethInterface == ETH_MII_REMAPED)
    ETHinterfaceRemapedMIIconfigure();
  else if (ethInterface == ETH_RMII)
    ETHinterfaceRMIIconfigure();
  else if (ethInterface == ETH_RMII_REMAPED)
    ETHinterfaceRemapedRMIIconfigure();
  else
    return -1;
  ETHpowerConfigure();
  ETHpowerUp();
  ETH_DeInit();
  ETH_SoftwareReset();
  active_check(ETH_GetSoftwareResetStatus() == RESET, maxAttempts);
  return 0;
}

void ETHinterfaceMIIconfigure() {
  /* Uzupełnij implementację. */
}

void ETHinterfaceRemapedMIIconfigure() {
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                         RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC |
                         RCC_APB2Periph_GPIOD |
                         /* RCC_APB2Periph_GPIOE | */
                         RCC_APB2Periph_AFIO,
                         ENABLE);

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ETH_MAC |
                        RCC_AHBPeriph_ETH_MAC_Tx |
                        RCC_AHBPeriph_ETH_MAC_Rx,
                        ENABLE);

  GPIO_PinRemapConfig(GPIO_Remap_ETH, ENABLE);
  GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_MII);

  /* Linie wejściowe
     ETH_MII_CRS    - PA0 - Odbieranie carrier sense jest wymagane
                            w half-duplex.
     ETH_MII_RX_CLK - PA1
     ETH_MII_COL    - PA3
     ETH_MII_RX_ER  - PB10
     ETH_MII_TX_CLK - PC3
     ETH_MII_RX_DV  - PD8
     ETH_MII_RXD0   - PD9
     ETH_MII_RXD1   - PD10
     ETH_MII_RXD2   - PD11
     ETH_MII_RXD3   - PD12 */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = 0;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 |
                                GPIO_Pin_10 | GPIO_Pin_11 |
                                GPIO_Pin_12;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Linie wyjściowe
     ETH_MII_MDIO    - PA2
     ETH_MII_PPS_OUT - PB5 - nie podłączone do PHY
     ETH_MII_TXD3    - PB8
     ETH_MII_TX_EN   - PB11
     ETH_MII_TXD0    - PB12
     ETH_MII_TXD1    - PB13
     ETH_MII_MDC     - PC1
     ETH_MII_TXD2    - PC2
     MCO             - PA8 - zegar jako ostatni */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = /* GPIO_Pin_5 | */
                                GPIO_Pin_8 | GPIO_Pin_11 |
                                GPIO_Pin_12 | GPIO_Pin_13;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void ETHinterfaceRMIIconfigure(void) {
  /* Uzupełnij implementację. */
}

void ETHinterfaceRemapedRMIIconfigure(void) {
  /* Uzupełnij implementację. */
}

/* Konfiguruj wyprowadzenie PWRDWN modułu ethernetowego (PHY). */
void ETHpowerConfigure() {
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(rccPowerDown, ENABLE);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Pin = gpioPowerDownPin;
  GPIO_Init(gpioPowerDown, &GPIO_InitStructure);
}

/* Wyłącz zasilanie modułu ethernetowego. */
void ETHpowerDown() {
  /* ETH_PowerDownCmd(ENABLE); */
  GPIO_WriteBit(gpioPowerDown, gpioPowerDownPin, Bit_SET);
}

/* Włącz zasilanie modułu ethernetowego. Po resecie wyjście jest
   w stanie niskim, ale na wszelki wypadek... */
void ETHpowerUp() {
  GPIO_WriteBit(gpioPowerDown, gpioPowerDownPin, Bit_RESET);
}
