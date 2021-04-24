#include <board_led.h>
#include <util_time.h>
#include <stdio.h>
#include <string.h>
#include <lwip/tcp.h>
#include <tcp_server.h>

#define ERR_EXIT 100
#define IAC      255    /* Interpret as command. */
#define DONT     254    /* You are not to use option. */
#define DO       253    /* Please, you use option. */
#define WONT     252    /* I won't use option. */
#define WILL     251    /* I will use option. */

static const char invitation[] =
  "\r\n"
  "This is TCP server running on top of the lwIP stack.\r\n"
  "Press h or ? to get help.\r\n"
  "\r\n"
  "> ";

static const char help[] =
  "\r\n"
  "h or ? print this help\r\n"
  "G      turn on the green led\r\n"
  "g      turn off the green led\r\n"
  "R      turn on the red led\r\n"
  "r      turn off the red led\r\n"
  "t      get microcontroller running time\r\n"
  "x      exit\r\n"
  "\r\n"
  "> ";

static const char prompt[] =
  "\r\n"
  "> ";

static const char error[] =
  "\r\n"
  "ERROR\r\n"
  "\r\n"
  "> ";

static const char timeout[] =
  "\r\n"
  "TIMEOUT\r\n";

static const char timefrm[] =
  "\r\n"
  "%u %02u:%02u:%02u.%03u\r\n"
  "\r\n"
  "> ";

#define SERVER_TIMEOUT 30
#define POLL_PER_SECOND 2
#define TIME_BUF_SIZE 32

#define tcp_write_from_rom(p, x) tcp_write(p, x, sizeof(x) - 1, 0)

struct state {
  err_t (* function)(struct state *, struct tcp_pcb *, uint8_t);
  int timeout;
};

static err_t
accept_callback(void *, struct tcp_pcb *, err_t);
static err_t
recv_callback(void *, struct tcp_pcb *, struct pbuf *, err_t);
static err_t
poll_callback(void *, struct tcp_pcb *);
static void
conn_err_callback(void *, err_t);
static void
tcp_exit(struct state *, struct tcp_pcb *);

static err_t
StateAutomaton(struct state *, struct tcp_pcb *, struct pbuf *);
static err_t
InitState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
IACState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
OptionState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
HelpState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
OnGreenLedState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
OffGreenLedState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
OnRedLedState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
OffRedLedState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
GetLocalTimeState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
ExitState(struct state *, struct tcp_pcb *, uint8_t);
static err_t
ErrorState(struct state *, struct tcp_pcb *, uint8_t);

/* Ta funkcja nie jest wołana w kontekście procedury obsługi
   przerwania. */
int TCPserverStart(uint16_t port) {
  struct tcp_pcb *pcb, *listen_pcb;
  err_t err;
  IRQ_DECL_PROTECT(x);

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  pcb = tcp_new();
  IRQ_UNPROTECT(x);
  if (pcb == NULL)
    return -1;

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  err = tcp_bind(pcb, IP_ADDR_ANY, port);
  IRQ_UNPROTECT(x);
  if (err != ERR_OK) {
    IRQ_PROTECT(x, LWIP_IRQ_PRIO);
    /* Trzeba zwolnić pamięć, poprzednio było tcp_abandon(pcb, 0); */
    tcp_close(pcb);
    IRQ_UNPROTECT(x);
    return -1;
  }

  /* Między wywołaniem tcp_listen a tcp_accept stos lwIP nie może
     odbierać połączeń. */
  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  listen_pcb = tcp_listen(pcb);
  if (listen_pcb)
    tcp_accept(listen_pcb, accept_callback);
  IRQ_UNPROTECT(x);

  if (listen_pcb == NULL) {
    IRQ_PROTECT(x, LWIP_IRQ_PRIO);
    /* Trzeba zwolnić pamięć, poprzednio było tcp_abandon(pcb, 0); */
    tcp_close(pcb);
    IRQ_UNPROTECT(x);
    return -1;
  }
  return 0;
}

err_t accept_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
  struct state *state;

  /* Z analizy kodu wynika, że zawsze err == ERR_OK.
  if (err != ERR_OK)
    return err;
  */

  state = mem_malloc(sizeof(struct state));
  if (state == NULL)
    return ERR_MEM;
  state->function = InitState;
  state->timeout = SERVER_TIMEOUT;
  tcp_arg(pcb, state);

  tcp_err(pcb, conn_err_callback);
  tcp_recv(pcb, recv_callback);
  tcp_poll(pcb, poll_callback, POLL_PER_SECOND);

  return tcp_write_from_rom(pcb, invitation);
}

err_t recv_callback(void *arg,
                    struct tcp_pcb *pcb,
                    struct pbuf *p,
                    err_t err) {
  /* Z analizy kodu wynika, że zawsze err == ERR_OK.
  if (err != ERR_OK)
    return err;
  */
  if (p) {
    /* Odebrano dane. */
    tcp_recved(pcb, p->tot_len);
    err = StateAutomaton(arg, pcb, p);
    if (err == ERR_OK || err == ERR_EXIT)
      pbuf_free(p);
    if (err == ERR_EXIT) {
      tcp_exit(arg, pcb);
      err = ERR_OK;
    }
  }
  else {
    /* Druga strona zamknęła połączenie. */
    tcp_exit(arg, pcb);
    err = ERR_OK;
  }
  return err;
}

err_t poll_callback(void *arg, struct tcp_pcb *pcb) {
  struct state *state = arg;

  if (state == NULL)
    /* Najwyraźniej połączenie nie zostało jeszcze zamknięte. */
    tcp_exit(arg, pcb);
  else if (--(state->timeout) <= 0) {
    tcp_write_from_rom(pcb, timeout);
    tcp_exit(arg, pcb);
  }
  return ERR_OK;
}

void conn_err_callback(void *arg, err_t err) {
  mem_free(arg);
}

void tcp_exit(struct state *state, struct tcp_pcb *pcb) {
  /* Po wywołaniu tcp_close() mogą jeszcze przyjść dane, ale nie
     można ich już obsłużyć po wywołaniu mem_free(), a po wywołaniu
     tcp_close() może już nie być szansy wywołania mem_free(). */
  tcp_recv(pcb, NULL);
  /* Ta funkcja może być wywołana wielokrotnie, np. przez
     poll_collback(), jeśli tcp_close() zawiedzie. */
  if (state) {
    mem_free(state);
    tcp_arg(pcb, NULL);
  }
  tcp_close(pcb);
}

err_t StateAutomaton(struct state *s,
                     struct tcp_pcb *pcb,
                     struct pbuf *p) {
  s->timeout = SERVER_TIMEOUT;
  for (;;) {
    uint8_t *c = (uint8_t *)p->payload;
    uint16_t i;
    err_t err;

    for (i = 0; i < p->len; ++i)
      if (ERR_OK != (err = s->function(s, pcb, c[i])))
        return err;
    if (p->len == p->tot_len)
      break;
    else
      p = p->next;
  }
  return ERR_OK;
}

err_t InitState(struct state *state,
                struct tcp_pcb *pcb,
                uint8_t c) {
  switch (c) {
    case IAC:
      state->function = IACState;
      return ERR_OK;
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      return tcp_write_from_rom(pcb, prompt);
    case 'h':
    case '?':
      state->function = HelpState;
      return ERR_OK;
    case 'G':
      state->function = OnGreenLedState;
      return ERR_OK;
    case 'g':
      state->function = OffGreenLedState;
      return ERR_OK;
    case 'R':
      state->function = OnRedLedState;
      return ERR_OK;
    case 'r':
      state->function = OffRedLedState;
      return ERR_OK;
    case 't':
      state->function = GetLocalTimeState;
      return ERR_OK;
    case 'x':
      state->function = ExitState;
      return ERR_OK;
    case '$':
      /* Zawieś serwer, żeby przetestować watchdog. */
      for(;;);
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

static err_t IACState(struct state *state,
                      struct tcp_pcb *pcb,
                      uint8_t c) {
  switch (c) {
    case DONT:
    case DO:
    case WONT:
    case WILL:
      state->function = OptionState;
      return ERR_OK;
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

static err_t OptionState(struct state *state,
                         struct tcp_pcb *pcb,
                         uint8_t c) {
   state->function = InitState;
   return ERR_OK;
}

static err_t HelpState(struct state *state,
                       struct tcp_pcb *pcb,
                       uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      state->function = InitState;
      return tcp_write_from_rom(pcb, help);
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t OnGreenLedState(struct state *state,
                      struct tcp_pcb *pcb,
                      uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      state->function = InitState;
      GreenLEDon();
      return tcp_write_from_rom(pcb, prompt);
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t OffGreenLedState(struct state *state,
                       struct tcp_pcb *pcb,
                       uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      state->function = InitState;
      GreenLEDoff();
      return tcp_write_from_rom(pcb, prompt);
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t OnRedLedState(struct state *state,
                    struct tcp_pcb *pcb,
                    uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      state->function = InitState;
      RedLEDon();
      return tcp_write_from_rom(pcb, prompt);
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t OffRedLedState(struct state *state,
                     struct tcp_pcb *pcb,
                     uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      state->function = InitState;
      RedLEDoff();
      return tcp_write_from_rom(pcb, prompt);
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t GetLocalTimeState(struct state *state,
                        struct tcp_pcb *pcb,
                        uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n': {
      char buf[TIME_BUF_SIZE];
      unsigned d, h, m, s, ms;
      int size;
      struct _reent reent;

      state->function = InitState;
      GetLocalTime(&d, &h, &m, &s, &ms);
      /* Funkcja snprintf nie jest współużywalna (ang. reentrant).
         Rozwiązanie nie jest przenośne, bo funkcja _snprintf_r nie
         należy do standardowej biblioteki C.
      size = snprintf(buf, TIME_BUF_SIZE, timefrm, d, h, m, s, ms);
      */
      size = _snprintf_r(&reent, buf, TIME_BUF_SIZE,
                         timefrm, d, h, m, s, ms);
      if (size <= 0)
        return tcp_write_from_rom(pcb, error);
      if (size >= TIME_BUF_SIZE)
        /* Nie wysyłaj terminalnego zera. */
        size = TIME_BUF_SIZE - 1;
      return tcp_write(pcb, buf, size, TCP_WRITE_FLAG_COPY);
    }
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t ExitState(struct state *state,
                struct tcp_pcb *pcb,
                uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
      return ERR_OK;
    case '\n':
      return ERR_EXIT;
    default:
      state->function = ErrorState;
      return ERR_OK;
  }
}

err_t ErrorState(struct state *state,
                 struct tcp_pcb *pcb,
                 uint8_t c) {
  switch (c) {
    case '\n':
      state->function = InitState;
      return tcp_write_from_rom(pcb, error);
    default:
      return ERR_OK;
  }
}
