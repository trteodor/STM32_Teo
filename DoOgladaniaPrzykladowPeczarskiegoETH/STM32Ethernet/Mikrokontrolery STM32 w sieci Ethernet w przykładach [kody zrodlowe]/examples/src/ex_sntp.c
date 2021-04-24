#include <board_conf.h>
#include <board_init.h>
#include <board_lcd.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_error.h>
#include <util_lcd.h>
#include <util_lcd_ex.h>
#include <util_lwip.h>
#include <util_rtc.h>
#include <util_time.h>
#include <sntp_client.h>

/* Aby przetestować aplikację, na liście jest serwer
   nieistniejący oraz serwer nieobsługujący NTP. */
#define COUNT  5
static const char * const serverTbl[COUNT] = {
  "wikipedia.org",
  "nie.ma.takiego.serwera",
  "wikipedia.org",
  "tempus1.gum.gov.pl",
  "tempus2.gum.gov.pl"
};
static int status[COUNT] = {
  SNTP_NOT_RUNNING,
  SNTP_NOT_RUNNING,
  SNTP_NOT_RUNNING,
  SNTP_NOT_RUNNING,
  SNTP_NOT_RUNNING
};

/* Wykonuje się w przerwaniu RTC. */
static void ClockCallback(void) {
  static int f = 0;
  int i;

  LCDgoto(0, 1);
  LCDwriteTime(GetRealTimeClock());
  for (i = 0; i < COUNT; ++i) {
    LCDgoto(2 + i, 0);
    if (f == 0) {
      LCDwrite(serverTbl[i]);
      LCDwrite("                     ");
    }
    else if (f == 2 || f == 3) {
      switch (status[i]) {
        case SNTP_NOT_RUNNING:
          LCDwrite("SNTP not running      ");
          break;
        case SNTP_SYNCHRONIZED:
          LCDwrite("Time synchronized     ");
          break;
        case SNTP_IN_PROGRESS:
          LCDwrite("SNTP in progress      ");
          break;
        case SNTP_DNS_ERROR:
          LCDwrite("DNS error             ");
          break;
        case SNTP_MEMORY_ERROR:
          LCDwrite("Memory error          ");
          break;
        case SNTP_UDP_ERROR:
          LCDwrite("UDP error             ");
          break;
        case SNTP_NO_RESPONSE:
          LCDwrite("SNTP no response      ");
          break;
        case SNTP_RESPONSE_ERROR:
          LCDwrite("SNTP response error   ");
          break;
      }
    }
  }
  f = (f + 1) & 3;
}

int main() {
  static struct netif netif;
  static struct ethnetif ethnetif = {PHY_ADDRESS};
  int i;
  uint8_t confBit;

  Delay(1000000);       /* Czekaj na JTAG ok. 0,5 s. */
  confBit = GetConfBit();
  AllPinsDisable();
  LEDconfigure();
  RedLEDon();
  LCDconfigure();
  SET_IRQ_PROTECTION();

  error_resetable(CLKconfigure(), 1);
  error_permanent(LocalTimeConfigure(), 2);
  error_resetable(RTCconfigure(), 3);
  RTCsetCallback(ClockCallback);
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
  LWIPsetAppTimer(SNTPtimer, SNTP_TIMER_MS);
  error_resetable(DHCPwait(&netif, 10, 4), 6);
  RedLEDoff();

  for (i = 0; i < COUNT; ++i) {
    Delay(10000000);
    SNTPclientStart(serverTbl[i], &status[i]);
  }
  for (;;);
}
