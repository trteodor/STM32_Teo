#include <board_def.h>
#include <board_conf.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>

#if BOARD_TYPE == PROTOTYPE || \
    BOARD_TYPE == BUTTERFLY || \
    BOARD_TYPE == ZL29ARM
  #define GPIO_CONF        GPIOB
  #define CONF_PIN         GPIO_Pin_2
  #define RCC_PERIPH_CONF  RCC_APB2Periph_GPIOB
#else
  # error Undefined BOARD_TYPE
#endif

uint8_t GetConfBit() {
  uint8_t bit;

  RCC_APB2PeriphClockCmd(RCC_PERIPH_CONF, ENABLE);
  bit = GPIO_ReadInputDataBit(GPIO_CONF, CONF_PIN);
  RCC_APB2PeriphClockCmd(RCC_PERIPH_CONF, DISABLE);
  return bit;
}
