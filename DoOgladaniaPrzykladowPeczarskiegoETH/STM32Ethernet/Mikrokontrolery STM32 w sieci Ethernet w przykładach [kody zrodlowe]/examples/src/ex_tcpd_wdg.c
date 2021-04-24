#include <board_conf.h>
#include <board_init.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_led.h>
#include <util_lwip.h>
#include <util_time.h>
#include <util_wdg.h>
#include <tcp_server.h>

#define WDG_TIMEOUT_MS          26000
#define WDG_RESET_INTERVAL_MS   13000

int main() {
  static struct netif netif;
  static struct ethnetif ethnetif = {PHY_ADDRESS};
  uint8_t confBit;

  Delay(1000000);       /* Czekaj na JTAG ok. 0,5 s. */
  WatchdogStart(WDG_TIMEOUT_MS);
  confBit = GetConfBit();
  AllPinsDisable();
  LEDconfigure();
  RedLEDon();
  SET_IRQ_PROTECTION();

  error_check(CLKconfigure(), 1);
  error_check(LocalTimeConfigure(), 2);
  error_check(ETHconfigureMII(), 4);

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
  error_check(LWIPinterfaceInit(&netif, &ethnetif), 5);
  LWIPtimersStart();

  /* Czekaj na konfigurację IP maksymalnie 10 sekund lub 4 próby,
     a po jej uzyskaniu spróbuj uruchomić serwer. */
  error_check(DHCPwait(&netif, 10, 4), 6);
  error_check(TCPserverStart(23), 7);

  /* Serwer został uruchomiony. Jeśli nic nie zawiesi się w lwIP,
     funkcja WatchdogReset będzie wołana regularnie. */
  LWIPsetAppTimer(WatchdogReset, WDG_RESET_INTERVAL_MS);
  RedLEDoff();
  for (;;);
}
