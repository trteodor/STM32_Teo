#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

caddr_t _sbrk  _PARAMS ((int));
int     _open  _PARAMS ((const char *, int, ...));
int     _close _PARAMS ((int));
int     _read  _PARAMS ((int, char *, int));
int     _write _PARAMS ((int, char *, int));
int     _lseek _PARAMS ((int, int, int));
int     _fstat _PARAMS ((int, struct stat *));

/* Register name faking - works in collusion with the linker. */
register char * stack_ptr asm ("sp");

caddr_t _sbrk(int incr) {
  extern char end asm ("end"); /* Defined by the linker. */
  static char *heap_end;
  char        *prev_heap_end;

  if (heap_end == NULL)
    heap_end = &end;

  prev_heap_end = heap_end;

  if (heap_end + incr > stack_ptr) {
    /* Some of the libstdc++-v3 tests rely upon detecting
       out of memory errors, so do not abort here.  */
    errno = ENOMEM;
    return (caddr_t)-1;
  }

  heap_end += incr;

  return (caddr_t)prev_heap_end;
}

int _open(const char *path, int flags, ...) {
  return 0;
}

int _close(int file) {
  return 0;
}

int _read(int file, char *ptr, int len) {
  return 0;
}

int _write(int file, char *ptr, int len) {
  return 0;
}

int _lseek(int file, int ptr, int dir) {
  return 0;
}

int _fstat(int file, struct stat *st) {
  return 0;
}
