#ifndef _UTIL_ETH_H
#define _UTIL_ETH_H 1

#include <board_def.h>
#include <stdint.h>
#include <lwip/netif.h>

#if ETH_BOARD == STE100P || ETH_BOARD == ZL3ETH
  #define PHY_ADDRESS 1
#else
  # error Undefined ETH_BOARD
#endif

struct ethnetif {
  uint8_t phyAddress;
  unsigned RX_packets;
  unsigned TX_packets;
  unsigned RX_errors;
  unsigned TX_errors;
};

err_t ETHinit(struct netif *);

#endif
