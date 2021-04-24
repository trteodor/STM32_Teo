#include <board_def.h>
#include <board_init.h>
#include <board_lcd.h>
#include <board_led.h>
#include <util_delay.h>
#include <util_lcd.h>
#include <util_led.h>
#include <stm32_eth.h>
#include <stdio.h>

#ifndef ETH_BOARD
  #error ETH_BOARD undefined
#endif

#if ETH_BOARD == STE100P || ETH_BOARD == ZL3ETH
  #define PHY_ADDRESS 1
#else
  # error Undefined ETH_BOARD
#endif

static int ETHconfigureRX(uint8_t);

int main() {
  unsigned pkts;

  Delay(1000000); /* Czekaj na JTAG ok. 0,5 s. */
  AllPinsDisable();
  LEDconfigure();
  RedLEDon();
  LCDconfigure();
  LCDwrite("CLK ");
  error_check(CLKconfigure(), 1);
  LCDwrite("PASS\n");
  LCDwrite("MII ");
  error_check(ETHconfigureMII(), 4);
  LCDwrite("PASS\n");
  LCDwrite("ETH ");
  error_check(ETHconfigureRX(PHY_ADDRESS), 5);
  LCDwrite("PASS\n");
  RedLEDoff(); /* Konfigurowanie zakończone */

  pkts = 0;
  for (;;) {
    static uint8_t packet[ETH_MAX_PACKET_SIZE];
    unsigned size;

    size = ETH_HandleRxPkt(packet);
    if (size >= 60) { /* Najkrótsza ramka bez FCS ma 60 oktetów. */
      char buffer[64];

      if (pkts == 0)
        LCDclear();
      pkts++;
      snprintf(buffer, sizeof(buffer),
               /* Numer pakietu i MAC nadawcy */
               "%-4u %02x%02x%02x%02x%02x%02x\n"
               /* Rozmiar pakietu i MAC odbiorcy */
               "%4u %02x%02x%02x%02x%02x%02x",
               pkts,
               packet[6], packet[7], packet[8],
               packet[9], packet[10], packet[11],
               size + 4, /* ETH_HandleRxPkt daje długość bez FCS. */
               packet[0], packet[1], packet[2],
               packet[3], packet[4], packet[5]);
      LCDgoto(4 * ((pkts - 1) & 1), 0);
      LCDwrite(buffer);
      if (packet[12] == 0x08 && packet[13] == 0x00) {
        /* Pakiet IP */
        snprintf(buffer, sizeof(buffer),
                 /* Typ pakietu i IP nadawcy */
                 "%02x%02x %u.%u.%u.%u        \n"
                 /* Protokół warstwy wyższej (wg. RFC 790: 1 = ICMP,
                    6 = TCP, 17 = UDP) i IP odbiorcy */
                 "%4u %u.%u.%u.%u        ",
                 packet[12], packet[13],
                 packet[26], packet[27], packet[28], packet[29],
                 packet[23],
                 packet[30], packet[31], packet[32], packet[33]);
      }
      else if (packet[12] == 0x08 && packet[13] == 0x06) {
        /* Pakiet ARP */
        snprintf(buffer, sizeof(buffer),
                 /* Typ pakietu i IP nadawcy */
                 "%02x%02x %u.%u.%u.%u        \n"
                 /* Operacja ARP i IP odbiorcy */
                 "%02x%02x %u.%u.%u.%u        ",
                 packet[12], packet[13],
                 packet[28], packet[29], packet[30], packet[31],
                 packet[20], packet[21],
                 packet[38], packet[39], packet[40], packet[41]);
      }
      else {
        /* Inny typ pakietu nie powinien się pojawić. */
        snprintf(buffer, sizeof(buffer),
                 /* Typ pakietu */
                 "%02x%02x                \n"
                 "                    ",
                 packet[12], packet[13]);
      }
      LCDgoto(4 * ((pkts - 1) & 1) + 2, 0);
      LCDwrite(buffer);
    }
  }
}

int ETHconfigureRX(uint8_t phyAddress) {
  #define ETHRXBUFNB 4

  static ETH_DMADESCTypeDef DMARxDscrTab[ETHRXBUFNB]
    __attribute__ ((aligned (4)));
  /* Długość buforów powinna wynosić ETH_MAX_PACKET_SIZE + VLAN_TAG,
     jeśli są używane ramki VLAN. */
  static uint8_t RxBuff[ETHRXBUFNB][ETH_MAX_PACKET_SIZE]
    __attribute__ ((aligned (4)));

  ETH_DMARxDescChainInit(DMARxDscrTab, &RxBuff[0][0], ETHRXBUFNB);

  ETH_InitTypeDef e;

  ETH_StructInit(&e);
  e.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
  e.ETH_ReceiveAll = ETH_ReceiveAll_Enable;
  if (ETH_Init(&e, phyAddress) == 0)
    return -1;

  ETH_Start();
  return 0;
}
