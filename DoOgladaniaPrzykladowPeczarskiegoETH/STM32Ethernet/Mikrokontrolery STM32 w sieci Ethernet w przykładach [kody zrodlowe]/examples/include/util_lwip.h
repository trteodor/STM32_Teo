#ifndef _UTIL_LWIP_H
#define _UTIL_LWIP_H 1

#include <util_eth.h>
#include <lwip/netif.h>

typedef void (*app_callback_t)(void);

int LWIPinterfaceInit(struct netif *, struct ethnetif *);
int DHCPwait(struct netif *, unsigned, unsigned);
void LWIPtimersStart(void);
void LWIPsetAppTimer(app_callback_t, unsigned);

#endif
