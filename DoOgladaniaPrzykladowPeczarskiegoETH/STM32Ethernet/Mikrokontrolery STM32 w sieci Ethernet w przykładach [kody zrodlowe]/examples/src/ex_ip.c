#include <board_conf.h>
#include <board_init.h>
#include <board_lcd.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_lcd.h>
#include <util_lcd_ex.h>
#include <util_led.h>
#include <util_lwip.h>
#include <util_time.h>
#include <stdio.h>

int main() {
  static struct netif netif;
  static struct ethnetif ethnetif = {PHY_ADDRESS};
  uint8_t confBit;

  Delay(1000000);       /* Czekaj na JTAG ok. 0,5 s. */
  confBit = GetConfBit();
  AllPinsDisable();
  LEDconfigure();
  RedLEDon();
  LCDconfigure();
  SET_IRQ_PROTECTION();

  LCDwrite("Clock ");
  error_check(CLKconfigure(), 1);
  LCDwrite("PASS\n");
  LCDwrite("Local Time ");
  error_check(LocalTimeConfigure(), 2);
  LCDwrite("PASS\n");
  LCDwrite("Ethernet ");
  error_check(ETHconfigureMII(), 4);
  LCDwrite("PASS\n");

  /* Każde urządzenie powinno mieć inny adres MAC. */
  LCDwrite("lwIP stack ");
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
  error_check(LWIPinterfaceInit(&netif, &ethnetif), 5);
  LWIPtimersStart();
  LCDwrite("PASS\n");

  /* Na uzyskanie adresu IP czekaj maksymalnie
     10 sekund lub 4 próby. */
  LCDwrite("IP ");
  error_check(DHCPwait(&netif, 10, 4), 6);
  LCDwriteIP(&netif.ip_addr);
  RedLEDoff();

  for (;;) {
    LCDgoto(6, 0);
    LCDwriteRXTX(ethnetif.RX_packets, ethnetif.RX_errors,
                 ethnetif.TX_packets, ethnetif.TX_errors);
    Delay(1000);
  }
}
