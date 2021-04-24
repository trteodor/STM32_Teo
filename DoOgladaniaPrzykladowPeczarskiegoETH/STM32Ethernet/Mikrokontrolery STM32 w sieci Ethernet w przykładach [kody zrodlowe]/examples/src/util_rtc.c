#include <arch/cc.h>
#include <misc.h>
#include <stm32f10x_bkp.h>
#include <stm32f10x_exti.h>
#include <stm32f10x_pwr.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_rtc.h>
#include <board_init.h>
#include <util_rtc.h>

#define MAGIC_NUMBER 1836547290
static uint32_t magicNumber;
static uint32_t standbyTime;
static void (*callback)(void) = NULL;

/* Konfiguruj zegar czasu rzeczywistego.
   Funkcja RTC_WaitForLastTask powinna być wołana przed każdym
   zapisem do rejestru RTC, ale można ją też wołać po zapisie,
   jeśli chcemy być pewni, że zapis się wykonał. */
int RTCconfigure() {
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,
                         ENABLE);
  PWR_BackupAccessCmd(ENABLE);

  if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
    /* Mikrokontroler obudził się z trybu czuwania.
       Nie ma potrzeby ponownego konfigurowania RTC - konfiguracja
       jest zachowywana po wybudzeniu ze stanu czuwania. */
    PWR_ClearFlag(PWR_FLAG_SB);
    RTC_WaitForSynchro();
  }
  else {
    /* Mikrokontroler został wyzerowany. Konfiguruj RTC, aby był
       taktowany kwarcem 32768 Hz i tyka z okresem 1 s. */
    BKP_DeInit();
    RCC_LSEConfig(RCC_LSE_ON);
    active_check(RCC_GetFlagStatus(RCC_FLAG_LSERDY), 10000000);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();
    RTC_SetPrescaler(32767);
    RTC_WaitForLastTask();
  }

  /* Konfiguruj przerwania RTC: alarm i zgłaszane co sekundę.
     Nie uaktywniaj przerwań. */
  EXTI_StructInit(&EXTI_InitStructure);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_Init(&EXTI_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
    LOW_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
  NVIC_Init(&NVIC_InitStructure);

  /* TODO: Można obudzić, podając stan wysoki na PA0. */
  /* PWR_WakeUpPinCmd(ENABLE); */

  return 0;
}

void RTCsetCallback(void (*f)(void)) {
  if (f == NULL) {
    /* Dezaktywuj przerwanie przed wyzerowaniem callback. */
    RTC_WaitForLastTask();
    RTC_ITConfig(RTC_IT_SEC, DISABLE);
  }

  callback = f;

  if (f) {
    /* Uaktywnij przerwanie zgłaszane co sekundę. */
    RTC_WaitForLastTask();
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
  }
}

uint32_t GetRealTimeClock() {
  RTC_WaitForLastTask();
  return RTC_GetCounter();
}

void SetRealTimeClock(uint32_t t) {
  RTC_WaitForLastTask();
  RTC_SetCounter(t);
}

/* Wejdź w stan czuwania na time sekund. */
void Standby(uint32_t time) {
  RTC_WaitForLastTask();
  RTC_SetAlarm(RTC_GetCounter() + time);
  RTC_WaitForLastTask();
  ETHpowerDown();
  PWR_EnterSTANDBYMode();
}

/* Wejdź w stan czuwania za delay sekund na time sekund. */
void DelayedStandby(uint32_t delay, uint32_t time) {
  if (delay > 0) {
    magicNumber = MAGIC_NUMBER;
    standbyTime = time;
    /* Alarm zgłaszany jest w ostatnim cyklu sygnału zegarowego
       taktującego RTC odpowiadającego sekundzie, w której alarm
       należy zgłosić, więc delay <= czas alarmu < delay + 1. */
    RTC_WaitForLastTask();
    RTC_SetAlarm(RTC_GetCounter() + delay);
    RTC_WaitForLastTask();
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
  }
  else
    Standby(time);
}

void RTCAlarm_IRQHandler(void) {
  if (RTC_GetITStatus(RTC_IT_ALR) != RESET)  {
    /* Znaczników właściwie nie musisz zerować, gdy zasypiasz. */
    EXTI_ClearITPendingBit(EXTI_Line17);
    RTC_WaitForLastTask();
    RTC_ClearITPendingBit(RTC_IT_ALR);

    if (magicNumber == MAGIC_NUMBER) {
      magicNumber = 0;
      Standby(standbyTime);
    }
  }
}

void RTC_IRQHandler(void) {
  if (RTC_GetITStatus(RTC_IT_SEC) != RESET) {
    RTC_WaitForLastTask();
    RTC_ClearITPendingBit(RTC_IT_SEC);
    if (callback)
      callback();
  }
}
