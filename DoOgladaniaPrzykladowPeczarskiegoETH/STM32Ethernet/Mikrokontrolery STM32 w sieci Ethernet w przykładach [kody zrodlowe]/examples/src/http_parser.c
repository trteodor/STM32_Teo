#include <http_parser.h>
#include <lwip/mem.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE  32

typedef enum {METHOD_STATE, URI_STATE, VERSION_STATE, PARAM_STATE,
              CONTENT_STATE, END_STATE, ERROR_STATE} state_t;

struct http_parser {
  state_t              state;
  method_t             method;
  char                 buffer[BUFFER_SIZE];
  int                  buffer_idx;
  char const * const * resource_tbl;
  int                  resource_len;
  int                  resource_idx;
  int                  content_len;
  char const * const * param_tbl;
  int                  param_len;
  char                 param_flag[1];
};

struct http_parser *
http_parser_new(char const * const * const res_tbl, int res_len,
                char const * const * const prm_tbl, int prm_len) {
  struct http_parser *s;

  s = mem_malloc(sizeof(struct http_parser) + prm_len - 1);
  if (s) {
    s->state = METHOD_STATE;
    s->method = 0;
    s->buffer_idx = 0;
    s->resource_tbl = res_tbl;
    s->resource_len = res_len;
    s->resource_idx = -1;
    s->content_len = 0;
    s->param_tbl = prm_tbl;
    s->param_len = prm_len;
    memset(s->param_flag, 0, prm_len);
  }
  return s;
}

void http_parser_delete(struct http_parser *s) {
  mem_free(s);
}

method_t http_parser_method(const struct http_parser *s) {
  /* Wartość ujemna nigdy nie będzie identyfikowała żadnej metody. */
  return s ? s->method : -1;
}

int http_parser_resource(const struct http_parser *s) {
  /* Wartość ujemna oznacza, że nie znaleziono zasobu. */
  return s ? s->resource_idx : -1;
}

int http_parser_param(const struct http_parser *s, int idx) {
  /* Zero oznacza, że nie znaleziono parametru. */
  return s && idx >= 0 && idx < s->param_len ?
         s->param_flag[idx] : 0;
}

status_t http_parser_do(struct http_parser *s, char x) {
  if (!s)
    return Internal_Server_Error_500;
  else if (s->buffer_idx < BUFFER_SIZE)
    s->buffer[s->buffer_idx++] = x;
  switch (s->state) {
    case METHOD_STATE:
      /* Parsuj metodę. */
      if (s->buffer_idx == 4 &&
          strncmp(s->buffer, "GET ", 4) == 0) {
        s->method = GET_METHOD;
        s->buffer_idx = 0;
        s->state = URI_STATE;
      }
      else if (s->buffer_idx == 5 &&
               strncmp(s->buffer, "POST ", 5) == 0) {
        s->method = POST_METHOD;
        s->buffer_idx = 0;
        s->state = URI_STATE;
      }
      else if (s->buffer_idx >= 5) {
        s->state = ERROR_STATE;
        return Bad_Request_400;
      }
      return Continue_100;
    case URI_STATE:
      /* Parsowanie URI jest bardzo uproszczone. */
      if (x == ' ') {
        int i;

        /* Spacja nie jest częścią URI, usuń ją z bufora. */
        if (s->buffer[s->buffer_idx - 1] == ' ')
          s->buffer_idx--;
        for (i = 0; i < s->resource_len; ++i) {
          if (strncmp(s->buffer, s->resource_tbl[i],
                      s->buffer_idx) == 0) {
            s->resource_idx = i;
            s->buffer_idx = 0;
            s->state = VERSION_STATE;
            return Continue_100;
          }
        }
        s->state = ERROR_STATE;
        return Not_Found_404;
      }
      else if (s->buffer_idx >= BUFFER_SIZE) {
        s->state = ERROR_STATE;
        return Bad_Request_400;
      }
      return Continue_100;
    case VERSION_STATE:
      /* Parsuj numer wersji, obsługuj tylko wersje 1.0 i 1.1. */
      if (s->buffer_idx == 10 &&
          (strncmp(s->buffer, "HTTP/1.0\r\n", 10) == 0 ||
           strncmp(s->buffer, "HTTP/1.1\r\n", 10) == 0)) {
        s->buffer_idx = 0;
        s->state = PARAM_STATE;
      }
      else if (s->buffer_idx >= 10) {
        s->state = ERROR_STATE;
        return HTTP_Version_Not_Supported_505;
      }
      return Continue_100;
    case PARAM_STATE:
      if (x == '\n') {
        /* Wykrywaj koniec nagłówka. */
        if (s->buffer_idx == 2 &&
            s->buffer[0] == '\r' &&
            s->buffer[1] == '\n') {
          if (s->content_len > 0)
            s->state = CONTENT_STATE;
          else {
            s->state = END_STATE;
            return OK_200;
          }
        }
        /* Poszukuj długości ciała komunikatu. */
        else if (s->buffer_idx >= 19 &&
                 strncmp(s->buffer, "Content-Length: ", 16) == 0 &&
                 s->buffer[s->buffer_idx - 2] == '\r' &&
                 s->buffer[s->buffer_idx - 1] == '\n') {
          int i;

          for (i = 16; i < s->buffer_idx - 2; ++i) {
            if (isdigit((int)s->buffer[i])) {
              s->content_len *= 10;
              s->content_len += s->buffer[i] - '0';
            }
            else {
              s->state = ERROR_STATE;
              return Bad_Request_400;
            }
          }
        }
        s->buffer_idx = 0;
      }
      return Continue_100;
    case CONTENT_STATE:
      /* Parsuj w sposób uproszczony dane z formularza. */
      s->content_len--;
      if (x == '&' || s->content_len == 0) {
        int i;

        /* Znak & nie jest częścią parametru, usuń go z bufora. */
        if (s->buffer[s->buffer_idx - 1] == '&')
          s->buffer_idx--;
        for (i = 0; i < s->param_len; ++i)
          if (strncmp(s->buffer, s->param_tbl[i],
                      s->buffer_idx) == 0)
            s->param_flag[i] = 1;
        s->buffer_idx = 0;
      }
      if (s->content_len == 0) {
        s->state = END_STATE;
        return OK_200;
      }
      return Continue_100;
    default:
      s->state = ERROR_STATE;
      return Internal_Server_Error_500;
  }
}
