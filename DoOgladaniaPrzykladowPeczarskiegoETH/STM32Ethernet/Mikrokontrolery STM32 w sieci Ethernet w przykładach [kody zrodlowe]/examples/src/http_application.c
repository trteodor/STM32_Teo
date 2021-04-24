#include <http_application.h>
#include <board_led.h>
#include <util_time.h>
#include <lwip/init.h>
#include <stdio.h>

#define MAIN_PAGE   0
#define STM32_LOGO  1
#define DONE_PAGE   2

const char * const resource_tbl[] = {
  "/",
  "/stm32_logo.gif",
  "/led"
};
const int resource_tbl_len =
  sizeof(resource_tbl) / sizeof(resource_tbl[0]);

#define LED_G  0
#define LED_R  1

const char * const parameter_tbl[] = {
  "LED=G",
  "LED=R"
};
const int parameter_tbl_len =
  sizeof(parameter_tbl) / sizeof(parameter_tbl[0]);

static status_t http_main_page(struct http *http) {
  static const char main_page_html[] =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "\r\n"
    "<html>"
      "<head><title>Przykładowy Serwer HTTP</title></head>"
      "<body>"
        "<center>"
          "<p><img src=\"stm32_logo.gif\" alt=\"STM32 LOGO\" /></p>"
          "<p><font size=\"6\">%u %02u:%02u:%02u.%03u</font></p>"
          "<hr />"
          "<form action=\"/led\" method=\"post\">"
            "<p>"
            "Czerwona dioda świecąca "
            "<input type=\"checkbox\" name=\"LED\" value=\"R\" %s/>"
            "</p>"
            "<p>"
            "Zielona dioda świecąca "
            "<input type=\"checkbox\" name=\"LED\" value=\"G\" %s/>"
            "</p>"
            "<p><input type=\"submit\" value=\"Zmień\" /></p>"
          "</form>"
          "<hr />"
          "<p>"
            "lwIP/%u.%u.%u CMSIS/%x.%x "
            "STM32F10x Standard Peripherals Library/%u.%u.%u"
            #ifdef _NEWLIB_VERSION
              " newlib/" _NEWLIB_VERSION
            #endif
          "</p>"
        "</center>"
      "</body>"
    "</html>";
  /* Po sformatowaniu przez snprintf może zajmować więcej miejsca. */
  static const size_t main_page_len = sizeof(main_page_html) + 12;

  int data_size;
  unsigned d, h, m, s, ms;
  char *data;
  struct _reent reent;

  data = http_data_new(http, main_page_len);
  if (data == NULL)
    return Internal_Server_Error_500;
  GetLocalTime(&d, &h, &m, &s, &ms);
  data_size = _snprintf_r(&reent, data,
                          main_page_len, main_page_html,
                          d, h, m, s, ms,
                          RedLEDstate() ? "checked " : "",
                          GreenLEDstate() ? "checked " : "",
                          LWIP_VERSION_MAJOR, LWIP_VERSION_MINOR,
                          LWIP_VERSION_REVISION,
                          __CM3_CMSIS_VERSION_MAIN,
                          __CM3_CMSIS_VERSION_SUB,
                          __STM32F10X_STDPERIPH_VERSION_MAIN,
                          __STM32F10X_STDPERIPH_VERSION_SUB1,
                          __STM32F10X_STDPERIPH_VERSION_SUB2);
  if (data_size <= 0)
    return Internal_Server_Error_500;
  else if ((size_t)data_size < main_page_len)
    /* Bufor nie jest pełny. */
    http_data_len(http, (size_t)data_size);
  else
    /* Bufor jest pełny. Nie wysyłaj terminalnego zera. */
    http_data_len(http, main_page_len - 1);
  return OK_200;
}

static status_t http_stm32_logo(struct http *http) {
  #include <stm32_logo.h>

  http_data_rom(http, stm32_logo_gif, stm32_logo_gif_len);
  return OK_200;
}

static status_t http_done_page(struct http *http,
                               const struct http_parser *parser) {
  static const char done_page_html[] =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "\r\n"
    "<html>"
      "<head><title>Przykładowy Serwer HTTP</title></head>"
      "<body>"
        "<center>"
          "<font size=\"4\">"
            "<p>Wykonano</p>"
            "<p><a href=\"/\">Powrót</a></p>"
          "</font>"
        "</center>"
      "</body>"
    "</html>";
  static const size_t done_page_len = sizeof(done_page_html) - 1;

  if (http_parser_param(parser, LED_G))
    GreenLEDon();
  else
    GreenLEDoff();
  if (http_parser_param(parser, LED_R))
    RedLEDon();
  else
    RedLEDoff();
  http_data_rom(http, done_page_html, done_page_len);
  return OK_200;
}

status_t make_http_answer(struct http *http,
                          const struct http_parser *parser) {
  method_t method;
  int      idx;

  method = http_parser_method(parser);
  idx = http_parser_resource(parser);
  if (idx == MAIN_PAGE && method == GET_METHOD)
    return http_main_page(http);
  else if (idx == STM32_LOGO && method == GET_METHOD)
    return http_stm32_logo(http);
  else if (idx == DONE_PAGE && method == POST_METHOD)
    return http_done_page(http, parser);
  else
    return Not_Found_404;
}
