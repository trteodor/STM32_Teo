#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H 1

#include <lwip/ip_addr.h>

int TCPclientStart(struct ip_addr *, uint16_t);

#endif
