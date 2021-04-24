#include <board_lcd.h>
#include <util_lcd.h>
#include <util_lcd_ex.h>
#include <lwip/inet.h>
#include <stdio.h>

/* Te funkcje nie są współużywalne. */

void LCDwriteIP(const struct ip_addr *ip) {
  struct in_addr in_ip;

  in_ip.s_addr = ip->addr;
  LCDwrite(inet_ntoa(in_ip));
}

void LCDwriteIPport(const struct ip_addr *ip, uint16_t port) {
  struct in_addr in_ip;
  char buf[8];

  in_ip.s_addr = ip->addr;
  LCDwrite(inet_ntoa(in_ip));
  snprintf(buf, sizeof(buf), ":%"U16_F, port);
  LCDwrite(buf);
}

void LCDwriteRXTX(unsigned rx_pkts, unsigned rx_errs,
                  unsigned tx_pkts, unsigned tx_errs) {
  char buf[60];

  snprintf(buf, sizeof(buf), "RX:%u err:%u\nTX:%u err:%u",
           rx_pkts, rx_errs, tx_pkts, tx_errs);
  LCDwrite(buf);
}

void LCDwriteTime(time_t rtime) {
  struct tm tm;
  char buf[20];

  if (gmtime_r(&rtime, &tm))
    if (strftime(buf, sizeof(buf), "%Y.%m.%d %H:%M:%S", &tm))
      LCDwrite(buf);
}
