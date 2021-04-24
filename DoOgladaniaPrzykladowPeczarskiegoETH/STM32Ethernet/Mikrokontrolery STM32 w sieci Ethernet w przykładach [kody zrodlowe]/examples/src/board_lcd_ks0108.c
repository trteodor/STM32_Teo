#include <board_def.h>
#include <board_lcd.h>
#include <font5x8.h>
#include <util_delay.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>

#ifndef BOARD_TYPE
  # error BOARD_TYPE undefined
#endif

#if BOARD_TYPE == ZL29ARM
  #define DATA_PORT GPIOE
  #define CTRL_PORT GPIOE
  #define DATA_RCC  RCC_APB2Periph_GPIOE
  #define CTRL_RCC  RCC_APB2Periph_GPIOE
  #define D0_D7     (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | \
                     GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | \
                     GPIO_Pin_6 | GPIO_Pin_7)
  #define RS_LINE   GPIO_Pin_8
  #define E_LINE    GPIO_Pin_9
  #define RW_LINE   GPIO_Pin_10
  #define CS1_LINE  GPIO_Pin_11
  #define CS2_LINE  GPIO_Pin_12
  #define D0_SHIFT  0
#elif BOARD_TYPE == PROTOTYPE
  #error PROTOTYPE board does not support LCD.
#elif BOARD_TYPE == BUTTERFLY
  #error BUTTERFLY does not support LCD.
#else
  # error Undefined BOARD_TYPE
#endif

#define DISPLAY_ON        0x3F
#define SET_ADDRESS_Y     0x40
#define SET_PAGE_X        0xB8
#define SET_START_LINE_Z  0xC0
#define BUSY_STATUS       0x80
/* Maksymalna liczba prób odczytu statusu w oczekiwaniu,
   aż przestanie być BUSY. */
#define MAX_ATTEMPTS      1000
/* Początkowe opóźnienie potrzebne po resecie - dobrane
   eksperymentalnie, ale z dużym zapasem */
#define INIT_DELAY        600

#define X_SEGMENTS        1
#define Y_SEGMENTS        2
#define PAGE_X_BITS       3
#define ADDRESS_Y_BITS    6
#define PAGE_X_MASK       ((1 << PAGE_X_BITS) - 1)
#define ADDRESS_Y_MASK    ((1 << ADDRESS_Y_BITS) - 1)
#define SCREEN_WIDTH      (Y_SEGMENTS * (1 << ADDRESS_Y_BITS))
#define SCREEN_HEIGHT     (X_SEGMENTS * (1 << PAGE_X_BITS))

/* Prymitywy sterujące liniami LCD. Operują bezpośrednio na
   rejestrach GPIO. Standard Peripherals Library nie dostarcza
   atomowej operacji modyfikacji kilku bitów GPIO, ani szybkiej
   operacji zmiany kierunku kilku linii GPIO. Korzystamy
   bezpośrednio z CMSIS. Dobry kompilator potrafi rozwinąć te
   funkcje w miejscu wołania i zoptymalizować instrukcje
   warunkowe, jeśli argument jest stałą. */

#define SET_BITS(x) ((uint32_t)(x))
#define CLR_BITS(x) ((uint32_t)(x) << 16)

static void E(int x) {
  if (x)
    CTRL_PORT->BSRR = E_LINE;
  else
    CTRL_PORT->BRR  = E_LINE;
}

static void RS(int x) {
  if (x)
    CTRL_PORT->BSRR = RS_LINE;
  else
    CTRL_PORT->BRR  = RS_LINE;
}

static void RW(int x) {
  if (x)
    CTRL_PORT->BSRR = RW_LINE;
  else
    CTRL_PORT->BRR  = RW_LINE;
}

/* Dla tego LCD aktywnym poziomem CS jest poziom wysoki. */

static void ChipUnselect() {
  CTRL_PORT->BSRR = CLR_BITS(CS1_LINE | CS2_LINE);
}

static void ChipSelect(int x, int y) {
  if (x == 0 && y == 0)
    CTRL_PORT->BSRR = SET_BITS(CS1_LINE) | CLR_BITS(CS2_LINE);
  else if (x == 0 && y == 1)
    CTRL_PORT->BSRR = SET_BITS(CS2_LINE) | CLR_BITS(CS1_LINE);
  else
    ChipUnselect();
}

static void DataOut(uint8_t x) {
  DATA_PORT->BSRR = CLR_BITS(~((uint32_t)x << D0_SHIFT) & D0_D7) |
                    SET_BITS( ((uint32_t)x << D0_SHIFT) & D0_D7);
}

static uint8_t DataIn(void) {
  return (DATA_PORT->IDR & D0_D7) >> D0_SHIFT;
}

/* Funkcje DataLinesIn i DataLinesOut nie rekonfigurują portu
   atomowo. Żaden inny proces lub procedura obsługi przerwania
   NIE MOGĄ modyfikować rejestru CRL lub CRH dla DATA_PORT. */

static void DataLinesIn(void) {
  uint32_t r;

  /* Rezystory podciągające wymuszają, że domyślnie odczytany
     status będzie oznaczał brak gotowości. 0x88888888U oznacza
     wejście z rezystorem podciągającym do zasilania lub
     ściągającym do masy. */
  DATA_PORT->BSRR = SET_BITS(D0_D7);
  #if D0_SHIFT < 8
    r = DATA_PORT->CRL;
    r &= ~(0xffffffffU << (4 * D0_SHIFT));
    r |=   0x88888888U << (4 * D0_SHIFT);
    DATA_PORT->CRL = r;
  #endif
  #if D0_SHIFT > 0
    r = DATA_PORT->CRH;
    r &= ~(0xffffffffU >> (32 - 4 * D0_SHIFT));
    r |=   0x88888888U >> (32 - 4 * D0_SHIFT);
    DATA_PORT->CRH = r;
  #endif
}

static void DataLinesOut(void) {
  uint32_t r;

  /* 0x11111111U oznacza wyjście przeciwsobne 10 MHz. */
  #if D0_SHIFT < 8
    r = DATA_PORT->CRL;
    r &= ~(0xffffffffU << (4 * D0_SHIFT));
    r |=   0x11111111U << (4 * D0_SHIFT);
    DATA_PORT->CRL = r;
  #endif
  #if D0_SHIFT > 0
    r = DATA_PORT->CRH;
    r &= ~(0xffffffffU >> (32 - 4 * D0_SHIFT));
    r |=   0x11111111U >> (32 - 4 * D0_SHIFT);
    DATA_PORT->CRH = r;
  #endif
}

/* KS0108 wymaga opóźnień 140 ns i 450 ns. Nawet dla maksymalnej
   częstotliwości taktowania rdzenia 72 MHz powinno wystarczyć
      #define T140 2
      #define T450 7
   Mój egzemplarz LCD działa z zerowymi wartościami.
   W razie problemów należy spróbować je zwiększyć. */
#define T140 0
#define T450 0

/** Podstawowe operacje **/

static uint8_t ReadByte(void) {
  uint8_t data;

  DataLinesIn(); /* Najpierw ustaw jako wejścia. */
  RW(1);         /* Potem wystaw sygnału READ. */
  Delay(T140);
  E(1);
  Delay(T450);
  data = DataIn();
  E(0);
  return data;
}

static void WriteByte(uint8_t data) {
  RW(0);          /* Najpierw wystaw sygnału WRITE. */
  DataLinesOut(); /* Potem ustaw jako wyjścia. */
  Delay(T140);
  E(1);
  DataOut(data);
  Delay(T450);
  E(0);
}

static uint8_t Status(void) {
  static int max_attempts = MAX_ATTEMPTS;
  int i;
  uint8_t status;

  RS(0); /* instrukcja */
  i = 0;
  do
    status = ReadByte();
  while (++i < max_attempts && (status & BUSY_STATUS));

  /* Jeśli LCD nie odpowiada, to skracamy czas następnego
     oczekiwania. To zabezpiecza przed marnowaniem czasu,
     gdy LCD nie działa. */
  if (status & BUSY_STATUS)
    max_attempts >>= 1;
  else
    max_attempts = MAX_ATTEMPTS;

  return status;
}

static void WriteInstruction(uint8_t data) {
  if (!(Status() & BUSY_STATUS))
    /* W funkcji Status zostało ustawione 0 na linii RS. */
    WriteByte(data);
}

static void WriteDataToRam(uint8_t data) {
  if (!(Status() & BUSY_STATUS)) {
    RS(1); /* dane */
    WriteByte(data);
  }
}

/** Implementacja interfejsu **/

/* Bieżące położenie "kursora", x to numer wiersza liczony od góry,
   a y to numer piksela w wierszu liczony od lewej */
static int x, y;
static void InternalGoTo(int, int);

void LCDconfigure() {
  int sx, sy;
  GPIO_InitTypeDef GPIO_InitStruct;

  RCC_APB2PeriphClockCmd(CTRL_RCC | DATA_RCC, ENABLE);

  /* Ustaw poziomy, które pojawią się na wyprowadzeniach po ich
     skonfigurowaniu. Na lini E dobrze jest zacząć od poziomu
     niskiego. Na RS jest to nieistotne, bo jest ustawiana przed
     każdą operacją.*/
  E(0);
  RS(0);
  ChipUnselect();

  /* Linie danych konfiguruj początkowo jako wejścia. */
  DataLinesIn();
  RW(1);

  /* I dopiero teraz konfiguruj wyprowadzenia jako wyjścia. */
  GPIO_InitStruct.GPIO_Pin = E_LINE | RS_LINE | RW_LINE |
                             CS1_LINE | CS2_LINE;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_Init(CTRL_PORT, &GPIO_InitStruct);

  /* Trzeba odczekać, zanim zacznie się wysyłać polecenia. */
  Delay(INIT_DELAY);
  LCDclear();
  for (sx = 0; sx < X_SEGMENTS; ++sx) {
    for (sy = 0; sy < Y_SEGMENTS; ++sy) {
      ChipSelect(sx, sy);
      WriteInstruction(DISPLAY_ON);
      ChipUnselect();
    }
  }
}

void LCDclear() {
  int ix, iy, sx, sy;

  x = y = 0;

  for (sx = 0; sx < X_SEGMENTS; ++sx) {
    for (sy = 0; sy < Y_SEGMENTS; ++sy) {
      ChipSelect(sx, sy);
      WriteInstruction(SET_START_LINE_Z);
      for (ix = 0; ix <= PAGE_X_MASK; ++ix) {
        WriteInstruction(SET_PAGE_X | ix);
        WriteInstruction(SET_ADDRESS_Y);
        for (iy = 0; iy <= ADDRESS_Y_MASK; ++iy)
          WriteDataToRam(0);
      }
      WriteInstruction(SET_PAGE_X);
      WriteInstruction(SET_ADDRESS_Y);
      ChipUnselect();
    }
  }
}

void LCDgoto(int textLine, int charPos) {
  InternalGoTo(textLine, charPos * (CHAR_WIDTH + 1));
}

void InternalGoTo(int new_x, int new_y) {
  x = new_x;
  y = new_y;
  ChipSelect(x >> PAGE_X_BITS, y >> ADDRESS_Y_BITS);
  WriteInstruction(SET_PAGE_X | (x & PAGE_X_MASK));
  WriteInstruction(SET_ADDRESS_Y | (y & ADDRESS_Y_MASK));
  ChipUnselect();
}

void LCDputchar(char c) {
  int i;

  if (c == '\n')
    LCDgoto(x + 1, 0); /* przejście do następnego wiersza */
  else if (c == '\r')
    LCDgoto(x, 0); /* powrót na początek wiersza */
  else if (c == '\t')
    LCDgoto(x, (y / (CHAR_WIDTH + 1) + 8) & ~7); /* tabulacja */
  else if (c >= FIRST_CHAR && c <= LAST_CHAR) {
    for (i = 0; i <= CHAR_WIDTH; ++i) {
      if (x >= 0 && x < SCREEN_HEIGHT &&
          y >= 0 && y < SCREEN_WIDTH) {
        if ((y & ADDRESS_Y_MASK) == 0)
          InternalGoTo(x, y); /* zmiana segmentu */
        ChipSelect(x >> PAGE_X_BITS, y >> ADDRESS_Y_BITS);
        if (i < CHAR_WIDTH)
          WriteDataToRam(font5x8[c - FIRST_CHAR][i]);
        else
          WriteDataToRam(0);
        ChipUnselect();
      }
      ++y;
    }
  }
}

void LCDputcharWrap(char c) {
  if (y + CHAR_WIDTH > SCREEN_WIDTH)
    LCDputchar('\n'); /* Nie ma miejsca na następny znak, zawiń. */
  LCDputchar(c);
}
