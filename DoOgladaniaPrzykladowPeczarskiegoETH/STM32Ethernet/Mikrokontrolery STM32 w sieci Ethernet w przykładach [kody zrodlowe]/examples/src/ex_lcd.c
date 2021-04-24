#include <board_lcd.h>
#include <font5x8.h>
#include <util_delay.h>
#include <util_lcd.h>

int main(void) {
  static const unsigned delay_time = 10000000;
  char c;
  int i, j;

  LCDconfigure();
  for (;;) {
    LCDwrite("\nTest LCD\n\n");
    for (c = FIRST_CHAR; c <= LAST_CHAR; ++c)
      LCDputcharWrap(c);
    Delay(delay_time);
    LCDclear();
    LCDwriteWrap("The text wraps around if it is"
                 " too long to fit the screen.");
    for (i = 2; i >= 0; --i) {
      LCDgoto(7 - i, i);
      for (j = i; j > 0; --j)
        LCDputchar('\t');
      LCDwrite("The text does not fit to the screen.");
    }
    Delay(delay_time);
    LCDclear();
  }
}
