#ifndef _HTTP_STATE_H
#define _HTTP_STATE_H 1

#include <string.h>

/* Struktura i funkcje zdefiniowane w http_server.c,
   ale potrzebne też w http_application.c. */

struct http;

char * http_data_new(struct http *, size_t);
void http_data_len(struct http *, size_t);
void http_data_rom(struct http *, const char *, size_t);

#endif
