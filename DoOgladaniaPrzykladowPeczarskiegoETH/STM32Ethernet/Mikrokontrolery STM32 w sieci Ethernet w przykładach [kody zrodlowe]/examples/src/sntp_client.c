/* Klient protokołu SNTP wg RFC 4330 */

/* To jest błąd: lwip/netif.h musi włączony przed lwip/dns.h. */
#include <lwip/netif.h>
#include <lwip/dns.h>
#include <lwip/udp.h>
#include <util_rtc.h>
#include <string.h>
#include <sntp_client.h>

#define NTP_PORT     123
#define NTP_TO_UNIX  2208988800U

PACK_STRUCT_BEGIN
struct ntp_timestamp {
  PACK_STRUCT_FIELD(uint32_t Seconds);
  PACK_STRUCT_FIELD(uint32_t SecondsFraction);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

PACK_STRUCT_BEGIN
struct ntp_msg {
  PACK_STRUCT_FIELD(uint8_t LI_VN_Mode);
  PACK_STRUCT_FIELD(uint8_t Stratum);
  PACK_STRUCT_FIELD(uint8_t Poll);
  PACK_STRUCT_FIELD(int8_t Precision);
  PACK_STRUCT_FIELD(uint32_t RootDelay);
  PACK_STRUCT_FIELD(uint32_t RootDispersion);
  PACK_STRUCT_FIELD(uint8_t ReferenceIdentifier[4]);
  PACK_STRUCT_FIELD(struct ntp_timestamp ReferenceTimestamp);
  PACK_STRUCT_FIELD(struct ntp_timestamp OriginateTimestamp);
  PACK_STRUCT_FIELD(struct ntp_timestamp ReceiveTimestamp);
  PACK_STRUCT_FIELD(struct ntp_timestamp TransmitTimestamp);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define NTP_LI_NO_WARNING          0
#define NTP_LI_ALARM_CONDITION     3
#define NTP_VERSION                4
#define NTP_MODE_CLIENT            3
#define NTP_MODE_SERVER            4
#define NTP_STRATUM_KISS_OF_DEATH  0
#define NTP_STRATUM_MAX            15

#define ntp_leap_indicator(pdu) ((uint8_t)((pdu)->LI_VN_Mode >> 6))
#define ntp_version(pdu) ((uint8_t)(((pdu)->LI_VN_Mode >> 3) & 0x7))
#define ntp_mode(pdu) ((uint8_t)((pdu)->LI_VN_Mode & 0x7))
#define ntp_compose_li_vn_mode(li, vn, mode)            \
 (((li) << 6) | (((vn) << 3) & 0x38) | ((mode) & 0x7))

struct sntp_client {
  struct sntp_client *next;
  struct udp_pcb     *pcb;
  int                timer;
  int                *status;
};

/* Czekaj na odpowiedź maksymalnie 4 sekundy. */
#define SNTP_TIMEOUT  20

static struct sntp_client *firstClient = NULL;

static void SNTPrequest(const char *, struct ip_addr *, void *);
static void SNTPreply(void *, struct udp_pcb *, struct pbuf *,
                      struct ip_addr *, u16_t);

void SNTPclientStart(const char *serverName, int *status) {
  struct ip_addr serverIP;
  err_t err;
  IRQ_DECL_PROTECT(x);

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  err = dns_gethostbyname(serverName, &serverIP, SNTPrequest,
                          (void*)status);
  IRQ_UNPROTECT(x);
  if (err == ERR_OK) {
    /* Odpowiedź odczytana z pamięci podręcznej. */
    IRQ_PROTECT(x, LWIP_IRQ_PRIO);
    SNTPrequest(serverName, &serverIP, (void*)status);
    IRQ_UNPROTECT(x);
    *status = SNTP_IN_PROGRESS;
  }
  else if (err == ERR_INPROGRESS)
    *status = SNTP_IN_PROGRESS;
  else
    *status = SNTP_DNS_ERROR;
}

/* Usuń klientów, którzy czekają zbyt długo. */
void SNTPtimer() {
  struct sntp_client *curr, *prev, *next;

  curr = firstClient;
  prev = NULL;
  while (curr) {
    if (--(curr->timer) < 0) {
      if (curr->pcb)
        udp_remove(curr->pcb);
      if (prev)
        next = prev->next = curr->next;
      else
        next = firstClient = curr->next;
      if (*(curr->status) == SNTP_IN_PROGRESS)
        *(curr->status) = SNTP_NO_RESPONSE;
      mem_free(curr);
      curr = next;
    }
    else {
      prev = curr;
      curr = curr->next;
    }
  }
}

void SNTPrequest(const char *serverName,
                 struct ip_addr *serverIP, void *status) {
  struct sntp_client *client;

  if (NULL == serverIP) {
    *(int*)status = SNTP_DNS_ERROR;
    return;
  }
  client = mem_malloc(sizeof(struct sntp_client));
  if (client == NULL) {
    *(int*)status = SNTP_MEMORY_ERROR;
    return;
  }

  client->pcb = NULL;
  client->timer = SNTP_TIMEOUT;
  client->status = (int*)status;
  client->next = firstClient;
  firstClient = client;

  if (NULL != (client->pcb = udp_new()) &&
      ERR_OK == udp_bind(client->pcb, IP_ADDR_ANY, 0) &&
      /* Odbieraj tylko odpowiedzi od serwera, do którego
         wysłałeś żądanie. */
      ERR_OK == udp_connect(client->pcb, serverIP, NTP_PORT)) {
    struct pbuf *p;

    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct ntp_msg), PBUF_RAM);
    if (p) {
      struct ntp_msg *msg = p->payload;

      memset(msg, 0, sizeof(struct ntp_msg));
      msg->LI_VN_Mode = ntp_compose_li_vn_mode(NTP_LI_NO_WARNING,
                                              NTP_VERSION,
                                              NTP_MODE_CLIENT);
      udp_recv(client->pcb, SNTPreply, client);
      if (ERR_OK != udp_send(client->pcb, p)) {
        *(int*)status = SNTP_UDP_ERROR;
        /* Wystąpił błąd, można przyspieszyć usunięcie klienta. */
        client->timer = 0;
      }
      pbuf_free(p);
    }
    else {
      *(int*)status = SNTP_MEMORY_ERROR;
      /* Wystąpił błąd, można przyspieszyć usunięcie klienta. */
      client->timer = 0;
    }
  }
  else {
    *(int*)status = SNTP_UDP_ERROR;
    /* Wystąpił błąd, można przyspieszyć usunięcie klienta. */
    client->timer = 0;
  }
}

void SNTPreply(void *arg, struct udp_pcb *pcb, struct pbuf *p,
               struct ip_addr *serverIP, u16_t serverPort) {
  struct sntp_client *client = arg;
  struct ntp_msg *msg;

  /* Jeśli otrzymano poprawną odpowiedź, uaktualnij czas.
     TODO: Uwzględnić: opóźnienie transmisji pakietów, strefy
     czasowe, zmiany na czas letni i zimowy, sekundy przestępne. */
  msg = p->payload;
  if (p->len >= sizeof(struct ntp_msg) &&
      ntp_leap_indicator(msg) != NTP_LI_ALARM_CONDITION &&
      ntp_version(msg) == NTP_VERSION &&
      ntp_mode(msg) == NTP_MODE_SERVER &&
      msg->Stratum != NTP_STRATUM_KISS_OF_DEATH &&
      msg->Stratum <= NTP_STRATUM_MAX &&
      (msg->TransmitTimestamp.Seconds != 0 ||
       msg->TransmitTimestamp.SecondsFraction != 0)) {
    SetRealTimeClock(ntohl(msg->TransmitTimestamp.Seconds) -
                     NTP_TO_UNIX);
    *(client->status) = SNTP_SYNCHRONIZED;
  }
  else
    *(client->status) = SNTP_RESPONSE_ERROR;
  pbuf_free(p);

  /* Klient już nie jest potrzebny. */
  client->timer = 0;
}
