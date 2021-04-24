#include <board_def.h>
#include <board_led.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>

#ifndef BOARD_TYPE
  #error BOARD_TYPE undefined
#endif

#if BOARD_TYPE == PROTOTYPE || \
    BOARD_TYPE == BUTTERFLY || \
    BOARD_TYPE == ZL29ARM
  #define GPIO_LED                 GPIOE
  #define RED_LED                  GPIO_Pin_15
  #define GREEN_LED                GPIO_Pin_14
  #define RCC_APB2Periph_GPIO_LED  RCC_APB2Periph_GPIOE
  #define LED_ON                   Bit_RESET
  #define LED_OFF                  Bit_SET
#else
  #error Undefined BOARD_TYPE
#endif

void LEDconfigure(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_LED, ENABLE);

  GPIO_WriteBit(GPIO_LED, RED_LED | GREEN_LED, LED_OFF);

  GPIO_StructInit(&GPIO_InitStruct);
  GPIO_InitStruct.GPIO_Pin = GREEN_LED | RED_LED;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIO_LED, &GPIO_InitStruct);
}

void RedLEDon(void) {
  GPIO_WriteBit(GPIO_LED, RED_LED, LED_ON);
}

void RedLEDoff(void) {
  GPIO_WriteBit(GPIO_LED, RED_LED, LED_OFF);
}

int RedLEDstate(void) {
  return GPIO_ReadOutputDataBit(GPIO_LED, RED_LED) == LED_ON;
}

void GreenLEDon(void) {
  GPIO_WriteBit(GPIO_LED, GREEN_LED, LED_ON);
}

void GreenLEDoff(void) {
  GPIO_WriteBit(GPIO_LED, GREEN_LED, LED_OFF);
}

int GreenLEDstate(void) {
  return GPIO_ReadOutputDataBit(GPIO_LED, GREEN_LED) == LED_ON;
}
