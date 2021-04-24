#include <board_conf.h>
#include <board_init.h>
#include <board_lcd.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_error.h>
#include <util_lcd.h>
#include <util_lwip.h>
#include <util_time.h>
#include <lwip/inet.h>
#include <lwip/udp.h>
#include <stdio.h>

#define SRC_PORT        5001
#define DST_PORT        5002
#define BCAST_TIMER_MS  2666
#define BUF_SIZE        64

static void SendBroadcast(void) {
  static unsigned count = 0;
  struct udp_pcb *pcb;
  struct pbuf *p;
  struct ip_addr broadcast_ip;
  struct in_addr ip;
  int size;
  char buf[BUF_SIZE];

  count++;
  if (count & 1)
    IP4_ADDR(&broadcast_ip, 192, 168,  51,  95);
  else
    IP4_ADDR(&broadcast_ip, 255, 255, 255, 255);
  ip.s_addr = broadcast_ip.addr;

  /* Funkcja snprintf nie jest współużywalna. */
  size = snprintf(buf, BUF_SIZE, "%u %s:%u\r\n",
                  count, inet_ntoa(ip), DST_PORT);
  if (size <= 0 || size >= BUF_SIZE)
    return;

  pcb = udp_new();
  if (pcb == NULL)
    return;

  if (ERR_OK != udp_bind(pcb, IP_ADDR_ANY, SRC_PORT) ||
      NULL == (p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM))) {
    udp_remove(pcb);
    return;
  }

  pbuf_take(p, buf, size);
  udp_sendto(pcb, p, &broadcast_ip, DST_PORT);
  pbuf_free(p);
  udp_remove(pcb);

  /* LCD nie jest współużywalny. */
  if ((count & 7) == 1)
    LCDclear();
  LCDwrite(buf);
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
  LCDconfigure();
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
  RedLEDoff(); /* Uruchamianie zostało zakończone. */

  LWIPsetAppTimer(SendBroadcast, BCAST_TIMER_MS);
  for (;;);
}
