#include <board_init.h>
#include <lwip/udp.h>
#include <udp_server.h>

#define PDU_VERSION       1
#define READ_INPUT_BITS   1
#define READ_OUTPUT_BITS  2
#define RESET_BITS        3
#define SET_BITS          4
#define WRITE_BITS        5
#define PDU_ERROR         8
#define INPUT_BITS        9
#define OUTPUT_BITS       10

PACK_STRUCT_BEGIN
struct pdu_t {
  PACK_STRUCT_FIELD(u8_t version_operation);
  PACK_STRUCT_FIELD(u8_t port);
  PACK_STRUCT_FIELD(u16_t data);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define pdu_version(pdu)   ((pdu)->version_operation >> 4)
#define pdu_operation(pdu) ((pdu)->version_operation & 0xf)
#define pdu_port(pdu)      ((pdu)->port)
#define pdu_data(pdu)      ntohs((pdu)->data)
#define set_pdu_version_operation(pdu, v, c) \
  ((pdu)->version_operation = ((v) << 4) | ((c) & 0xf))
#define set_pdu_port(pdu, x) ((pdu)->port = (x))
#define set_pdu_data(pdu, x) ((pdu)->data = htons(x))

static void recv_callback(void *, struct udp_pcb *, struct pbuf *,
                          struct ip_addr *, uint16_t);

/* Tylko ta funkcja nie jest wołana w kontekście procedury obsługi
   przerwania. */
int UDPserverStart(uint16_t port) {
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

void recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                   struct ip_addr *addr, uint16_t port) {
  struct pbuf *q;
  struct pdu_t *src, *dst;

  /* Potrzebny jest bufor do wysłania odpowiedzi. Nie jest jasne,
     dlaczego próba użycia w tym celu p nie działa. */
  q = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct pdu_t), PBUF_RAM);
  if (q == NULL) {
    pbuf_free(p);
    return;
  }

  src = p->payload;
  dst = q->payload;

  if (p->len < sizeof(struct pdu_t) ||
      pdu_version(src) != PDU_VERSION ||
      pdu_port(src) < 1 ||
      pdu_port(src) > gpioCount) {
    set_pdu_version_operation(dst, PDU_VERSION, PDU_ERROR);
    set_pdu_port(dst, 0);
    set_pdu_data(dst, 0);
  }
  else {
    GPIO_TypeDef *io = gpio[pdu_port(src) - 1];

    switch (pdu_operation(src)) {
      case READ_INPUT_BITS:
        set_pdu_version_operation(dst, PDU_VERSION, INPUT_BITS);
        set_pdu_port(dst, pdu_port(src));
        set_pdu_data(dst, GPIO_ReadInputData(io));
        break;
      case READ_OUTPUT_BITS:
        set_pdu_version_operation(dst, PDU_VERSION, OUTPUT_BITS);
        set_pdu_port(dst, pdu_port(src));
        set_pdu_data(dst, GPIO_ReadOutputData(io));
        break;
      case RESET_BITS:
        GPIO_ResetBits(io, pdu_data(src));
        set_pdu_version_operation(dst, PDU_VERSION, OUTPUT_BITS);
        set_pdu_port(dst, pdu_port(src));
        set_pdu_data(dst, GPIO_ReadOutputData(io));
        break;
      case SET_BITS:
        GPIO_SetBits(io, pdu_data(src));
        set_pdu_version_operation(dst, PDU_VERSION, OUTPUT_BITS);
        set_pdu_port(dst, pdu_port(src));
        set_pdu_data(dst, GPIO_ReadOutputData(io));
        break;
      case WRITE_BITS:
        GPIO_Write(io, pdu_data(src));
        set_pdu_version_operation(dst, PDU_VERSION, OUTPUT_BITS);
        set_pdu_port(dst, pdu_port(src));
        set_pdu_data(dst, GPIO_ReadOutputData(io));
        break;
      default:
        set_pdu_version_operation(dst, PDU_VERSION, PDU_ERROR);
        set_pdu_port(dst, 0);
        set_pdu_data(dst, 0);
    }
  }

  pbuf_free(p);
  udp_sendto(pcb, q, addr, port);
  pbuf_free(q);
}
