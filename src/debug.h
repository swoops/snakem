#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /*sleep*/
#include <stdarg.h> /*va_start*/
#define DEBUG_PAUSE 1
#define DEBUG_SLEEP 2

void debug(int flags, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(FDOUT, "\e[H\e[%dB\e[2K\e[48;5;88;38;5;0m", SERVER.max_y+2);
  va_end(ap);
  vfprintf(FDOUT, fmt, ap);

  /* not reliable now that server is implemented */
  /* 
  if ( flags & DEBUG_PAUSE )
    getchar();
  */

  if ( flags & DEBUG_SLEEP )
    sleep(5);

  // restore cursor to play area
  fprintf(FDOUT, "\e[00m\e[H");
}
