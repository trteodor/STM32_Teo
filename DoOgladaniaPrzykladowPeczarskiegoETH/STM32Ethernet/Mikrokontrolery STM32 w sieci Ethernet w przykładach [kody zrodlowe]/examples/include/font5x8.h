#ifndef _FONT5X8_H
#define _FONT5X8_H 1

/* Tablica znaków o kodach ASCII od 32 do 126 */

#define FIRST_CHAR 32
#define LAST_CHAR  126
#define CHAR_WIDTH 5

extern const char font5x8[LAST_CHAR - FIRST_CHAR + 1][CHAR_WIDTH];

#endif
