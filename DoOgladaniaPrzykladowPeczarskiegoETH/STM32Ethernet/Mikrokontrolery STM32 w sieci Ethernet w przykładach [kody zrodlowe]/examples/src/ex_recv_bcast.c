#include <board_conf.h>
#include <board_init.h>
#include <board_lcd.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_error.h>
#include <util_lcd.h>
#include <util_lcd_ex.h>
#include <util_lwip.h>
#include <util_time.h>
#include <lwip/udp.h>

#define UDP_PORT  5002

static void recv_callback(void *arg, struct udp_pcb *pcb,
                          struct pbuf *p, struct ip_addr *addr,
                          uint16_t port) {
  struct pbuf *q;
  uint16_t len;

  /* LCD nie jest współużywalny. */
  LCDclear();
  LCDwriteIPport(addr, port);
  LCDputchar('\n');
  for (len = 0, q = p; len < p->tot_len; len += q->len, q = q->next)
    LCDwriteLenWrap(q->payload, q->len);
  pbuf_free(p);
}

/* Ta funkcja nie jest wołana w kontekście procedury obsługi
   przerwania. */
static int UDPserverStart(uint16_t port) {
  struct udp_pcb *pcb;
  err_t err;
  IRQ_DECL_PROTECT(x);

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  pcb = udp_new();
  IRQ_UNPROTECT(x);
  if (pcb == NULL)
    return -1;

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  /* Ostatni parametr ma wartość NULL, bo serwer jest bezstanowy. */
  udp_recv(pcb, recv_callback, NULL);
  IRQ_UNPROTECT(x);

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  err = udp_bind(pcb, IP_ADDR_ANY, port);
  IRQ_UNPROTECT(x);
  if (err != ERR_OK) {
    IRQ_PROTECT(x, LWIP_IRQ_PRIO);
    udp_remove(pcb);
    IRQ_UNPROTECT(x);
    return -1;
  }
  return 0;
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
  netif.hwaddr[5] = 3 + confBit;
  if (!confBit) {
    IP4_ADDR(&netif.ip_addr, 192, 168,  51,  85);
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

  error_resetable(UDPserverStart(UDP_PORT), 7);
  RedLEDoff(); /* Serwer został uruchomiony. */
  for (;;);
}
