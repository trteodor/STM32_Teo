#include <lwip/tcp.h>
#include <http.h>
#include <http_application.h>
#include <http_parser.h>
#include <http_server.h>

/** Obsługa komunikacji HTTP **/

/* 10 sekund na odebranie żądania i odesłanie odpowiedzi. */
#define SERVER_TIMEOUT  10

/* Pamięć wskazywana przez składową data struktury http musi
   być zwolniona za pomocą mem_free. */
#define FLAG_NEEDS_FREE  1

struct http {
  struct http_parser *parser;
  status_t           status;
  int                timeout;
  char               *data;
  size_t             data_idx;
  size_t             data_left;
  uint32_t           data_flags;
};

static struct http * http_new(void);
static void http_delete(struct http *);
static void http_data_delete(struct http *);
static int  http_sent(struct http *);
static int  http_timeout(struct http *);
static void http_send(struct http *, struct tcp_pcb *);
static void http_engine(struct http *, struct tcp_pcb *,
                        struct pbuf *);
static void http_error_response(struct http *);

/** Obsługa połączenia TCP **/

#define POLL_PER_SECOND  2

static err_t accept_callback(void *, struct tcp_pcb *, err_t);
static err_t recv_callback(void *, struct tcp_pcb *,
                           struct pbuf *, err_t);
static err_t sent_callback(void *, struct tcp_pcb *, u16_t);
static err_t poll_callback(void *, struct tcp_pcb *);
static void conn_err_callback(void *, err_t);
static void close_connection(struct http *, struct tcp_pcb *);

/** Obsługa komunikacji HTTP - implementacja **/

struct http * http_new() {
  struct http_parser *parser;
  struct http *http;

  parser = http_parser_new(resource_tbl, resource_tbl_len,
                           parameter_tbl, parameter_tbl_len);
  if (parser == NULL)
    return NULL;
  http = mem_malloc(sizeof(struct http));
  if (http) {
    http->parser = parser;
    http->data = NULL;
    http->data_flags = 0;
    http->data_idx = 0;
    http->data_left = 0;
    http->status = Continue_100;
    http->timeout = SERVER_TIMEOUT;
  }
  else
    http_parser_delete(parser);
  return http;
}

void http_delete(struct http *http) {
  http_parser_delete(http->parser);
  if (http->data_flags & FLAG_NEEDS_FREE)
    mem_free(http->data);
  mem_free(http);
}

char * http_data_new(struct http *http, size_t size) {
  http->data = mem_malloc(size);
  if (http->data) {
    http->data_idx = 0;
    http->data_left = size;
    http->data_flags = FLAG_NEEDS_FREE;
  }
  return http->data;
}

void http_data_delete(struct http *http) {
  if (http->data_flags & FLAG_NEEDS_FREE) {
    mem_free(http->data);
    http->data = NULL;
    http->data_idx = 0;
    http->data_left = 0;
    http->data_flags = 0;
  }
}

void http_data_len(struct http *http, size_t size) {
  http->data_left = size;
}

void http_data_rom(struct http *http, const char *data,
                   size_t size) {
  http->data = (char *)data;
  http->data_idx = 0;
  http->data_left = size;
  http->data_flags = 0;
}

int http_sent(struct http *http) {
  return http->data_left == 0;
}

int http_timeout(struct http *http) {
  return --(http->timeout) <= 0;
}

void http_send(struct http *http, struct tcp_pcb *pcb) {
  uint16_t len;
  uint8_t  flag;

  len = tcp_sndbuf(pcb);
  if (len > 0 && http->data && http->data_left > 0) {
    if (len >= http->data_left) {
      /* Można wysłać wszystkie dane. */
      len = http->data_left;
      flag = 0;
    }
    else
      /* Pozostaną jeszcze jakieś dane do wysłania. */
      flag = TCP_WRITE_FLAG_MORE;
    if (ERR_OK == tcp_write(pcb, http->data + http->data_idx,
                            len, flag)) {
      http->data_idx  += len;
      http->data_left -= len;
    }
  }
}

void http_engine(struct http *http, struct tcp_pcb *pcb,
                 struct pbuf *p) {
  struct pbuf *q;

  if (http->status == Continue_100) {
    for (q = p;; q = q->next) {
      uint8_t *c = (uint8_t *)q->payload;
      uint16_t i;

      for (i = 0; i < q->len; ++i) {
        http->status = http_parser_do(http->parser, c[i]);
        if (http->status == Continue_100)
          continue;
        else if (http->status == OK_200) {
          /* Przygotuj odpowiedź. */
          http->status = make_http_answer(http, http->parser);
          if (http->status != OK_200)
            http_data_delete(http);
        }
        if (http->status != OK_200)
          http_error_response(http);
        http_send(http, pcb);
        return;
      }
      if (q->len == q->tot_len)
        break;
    }
  }
}

void http_error_response(struct http *http) {
  static const char bad_request_400_html[] =
    "HTTP/1.1 400 Bad Request\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
      "<head><title>Error</title></head>"
      "<body><h1>400 Bad Request</h1></body>"
    "</html>";
  static const char not_found_404_html[] =
    "HTTP/1.1 404 Not Found\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
      "<head><title>Error</title></head>"
      "<body><h1>404 Not Found</h1></body>"
    "</html>";
  static const char internal_server_error_500_html[] =
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
      "<head><title>Error</title></head>"
      "<body><h1>500 Internal Server Error</h1></body>"
    "</html>";
  static const char version_not_supported_505_html[] =
    "HTTP/1.1 505 HTTP Version Not Supported\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
      "<head><title>Error</title></head>"
      "<body><h1>505 HTTP Version Not Supported</h1></body>"
    "</html>";

  switch(http->status) {
    case Bad_Request_400:
      http_data_rom(http, bad_request_400_html,
                    sizeof(bad_request_400_html) - 1);
      return;
    case Not_Found_404:
      http_data_rom(http, not_found_404_html,
                    sizeof(not_found_404_html) - 1);
      return;
    case HTTP_Version_Not_Supported_505:
      http_data_rom(http, version_not_supported_505_html,
                    sizeof(version_not_supported_505_html) - 1);
      return;
    default:
      http_data_rom(http, internal_server_error_500_html,
                    sizeof(internal_server_error_500_html) - 1);
      return;
  }
}

/** Obsługa połączenia TCP - implementacja **/

/* Ta funkcja nie jest wołana w kontekście procedury obsługi
   przerwania. */
int HTTPserverStart(uint16_t port) {
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
  struct http *http;

  /* Z analizy kodu wynika, że zawsze err == ERR_OK.
  if (err != ERR_OK)
    return err;
  */
  http = http_new();
  if (http == NULL)
    return ERR_MEM;
  tcp_arg(pcb, http);
  tcp_err(pcb, conn_err_callback);
  tcp_recv(pcb, recv_callback);
  tcp_sent(pcb, sent_callback);
  tcp_poll(pcb, poll_callback, POLL_PER_SECOND);
  return ERR_OK;
}

err_t recv_callback(void *http, struct tcp_pcb *pcb,
                    struct pbuf *p, err_t err) {
  /* Z analizy kodu wynika, że zawsze err == ERR_OK.
  if (err != ERR_OK)
    return err;
  */
  if (p) {
    /* Odebrano dane. */
    tcp_recved(pcb, p->tot_len);
    http_engine(http, pcb, p);
    pbuf_free(p);
  }
  else
    /* Druga strona zamknęła połączenie. */
    close_connection(http, pcb);
  return ERR_OK;
}

err_t sent_callback(void *http, struct tcp_pcb *pcb, u16_t len) {
  if (http_sent(http))
    close_connection(http, pcb);
  else
    http_send(http, pcb);
  return ERR_OK;
}

err_t poll_callback(void *http, struct tcp_pcb *pcb) {
  if (http_timeout(http))
    close_connection(http, pcb);
  /* Można też wysyłać tu.
  else
    http_send(http, pcb);
  */
  return ERR_OK;
}

void conn_err_callback(void *http, err_t err) {
  http_delete(http);
}

void close_connection(struct http *http, struct tcp_pcb *pcb) {
  tcp_recv(pcb, NULL);
  tcp_sent(pcb, NULL);
  tcp_poll(pcb, NULL, 0);
  http_delete(http);
  tcp_arg(pcb, NULL);
  tcp_close(pcb);
}
