#ifndef _BOARD_LCD_H
#define _BOARD_LCD_H 1

void LCDconfigure(void);
void LCDclear(void);
void LCDgoto(int, int);
void LCDputchar(char);
void LCDputcharWrap(char);

#endif
