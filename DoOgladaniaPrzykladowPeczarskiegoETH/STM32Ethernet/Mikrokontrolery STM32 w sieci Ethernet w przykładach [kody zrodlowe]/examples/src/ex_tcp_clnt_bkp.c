#include <board_conf.h>
#include <board_init.h>
#include <board_led.h>
#include <util_bkp.h>
#include <util_delay.h>
#include <util_error.h>
#include <util_lwip.h>
#include <util_rtc.h>
#include <util_time.h>
#include <tcp_client.h>

int main() {
  static struct netif netif;
  static struct ethnetif ethnetif = {PHY_ADDRESS};
  static struct ip_addr addr;
  uint8_t confBit;

  Delay(1000000);       /* Czekaj na JTAG ok. 0,5 s. */
  confBit = GetConfBit();
  AllPinsDisable();
  LEDconfigure();
  RedLEDon();
  SET_IRQ_PROTECTION();
  BKPconfigure();

  error_resetable(CLKconfigure(), 1);
  error_permanent(LocalTimeConfigure(), 2);
  error_resetable(RTCconfigure(), 3);
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
  error_temporary(LWIPinterfaceInit(&netif, &ethnetif), 5);
  LWIPtimersStart();

  /* Przed uruchomieniem klienta poczekaj na uzyskanie
     adresu IP maksymalnie 10 sekund lub 4 próby. */
  error_temporary(DHCPwait(&netif, 10, 4), 6);
  IP4_ADDR(&addr, 192, 168,  51,  88);
  error_temporary(TCPclientStart(&addr, 22222), 7);

  /* Uruchamianie zostało zakończone. */
  RedLEDoff();
  for (;;);
}
