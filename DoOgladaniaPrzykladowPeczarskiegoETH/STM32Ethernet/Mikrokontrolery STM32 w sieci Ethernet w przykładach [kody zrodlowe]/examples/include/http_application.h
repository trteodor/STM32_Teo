#ifndef _HTTP_RESOURCES_H
#define _HTTP_RESOURCES_H 1

#include <http.h>
#include <http_parser.h>

extern const char * const resource_tbl[];
extern const int resource_tbl_len;
extern const char * const parameter_tbl[];
extern const int parameter_tbl_len;

status_t make_http_answer(struct http *, const struct http_parser *);

#endif
