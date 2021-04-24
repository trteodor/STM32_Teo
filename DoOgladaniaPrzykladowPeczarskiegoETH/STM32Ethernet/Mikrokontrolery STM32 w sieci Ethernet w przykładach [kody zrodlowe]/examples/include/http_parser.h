#ifndef _HTTP_PARSER_H
#define _HTTP_PARSER_H 1

/* Identyfikatory metod - muszą mieć wartości dodatnie. */
typedef enum {GET_METHOD = 1, POST_METHOD = 2} method_t;

/* Kody statusu HTTP. */
typedef enum {Continue_100                   = 100,
              OK_200                         = 200,
              Bad_Request_400                = 400,
              Not_Found_404                  = 404,
              Internal_Server_Error_500      = 500,
              HTTP_Version_Not_Supported_505 = 505} status_t;

struct http_parser;

struct http_parser *http_parser_new(char const * const * const, int,
                                    char const * const * const, int);
void http_parser_delete(struct http_parser *);
method_t http_parser_method(const struct http_parser *);
int http_parser_resource(const struct http_parser *);
int http_parser_param(const struct http_parser *, int);
status_t http_parser_do(struct http_parser *, char);

#endif
