#include <stdio.h>

int main() {
  int c, i;

  printf("const char ?????[] =\n");
  i = 0;
  while (EOF != (c = getchar())) {
    if (c < 0 || c > 255) {
      fprintf(stderr, "ERROR\n");
      return 1;
    }
    else if (i == 0) {
      printf("  \"\\x%02x", c);
      ++i;
    }
    else if (i == 15) {
      printf("\\x%02x\"\n", c);
      i = 0;
    }
    else {
      printf("\\x%02x", c);
      ++i;
    }
  }
  if (i != 15)
    printf("\";\n");
  else
    printf(";\n");
  return 0;
}
