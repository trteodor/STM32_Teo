#include <board_conf.h>
#include <board_init.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_error.h>
#include <util_lwip.h>
#include <util_time.h>
#include <udp_server.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>

/* Uruchom port C w trybie wejściowym. W ZL29ARM do PC5-PC9
   podłączony jest joystick, a do PC12 - SW3, PC13 - SW4. */
static void PORTCconfigure(void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_StructInit(&GPIO_InitStruct);
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 |
                             GPIO_Pin_8 | GPIO_Pin_9 |
                             GPIO_Pin_12 | GPIO_Pin_13;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOC, &GPIO_InitStruct);
}

int main() {
  static struct netif netif;
  static struct ethnetif ethnetif = {PHY_ADDRESS};
  uint8_t confBit;

  Delay(1000000);       /* Czekaj na JTAG ok. 0,5 s. */
  confBit = GetConfBit();
  AllPinsDisable();
  LEDconfigure();
  RedLEDon();
  SET_IRQ_PROTECTION();

  error_resetable(CLKconfigure(), 1);
  error_permanent(LocalTimeConfigure(), 2);
  error_resetable(ETHconfigureMII(), 4);

  /* Każde urządzenie powinno mieć inny adres MAC. */
  netif.hwaddr[0] = 2; /* adres MAC administrowany lokalnie */
  netif.hwaddr[1] = (BOARD_TYPE >> 8) & 0xff;
  netif.hwaddr[2] = BOARD_TYPE & 0xff;
  netif.hwaddr[3] = (ETH_BOARD >> 8) & 0xff;
  netif.hwaddr[4] = ETH_BOARD & 0xff;
  netif.hwaddr[5] = 1 + confBit;
  if (!confBit) {
    IP4_ADDR(&netif.ip_addr, 192, 168,  51,  84);
    IP4_ADDR(&netif.netmask, 255, 255, 255, 240);
    IP4_ADDR(&netif.gw,      192, 168,  51,  81);
  }
  else {
    /* Użyj DHCP. */
    IP4_ADDR(&netif.ip_addr, 0, 0, 0, 0);
    IP4_ADDR(&netif.netmask, 0, 0, 0, 0);
    IP4_ADDR(&netif.gw,      0, 0, 0, 0);
  }
  error_resetable(LWIPinterfaceInit(&netif, &ethnetif), 5);
  LWIPtimersStart();
  error_resetable(DHCPwait(&netif, 10, 4), 6);
  error_resetable(UDPserverStart(33333), 7);

  /* Serwer został uruchomiony. */
  RedLEDoff();
  PORTCconfigure();
  for (;;);
}
