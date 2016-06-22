#include  <stdarg.h>      /*va_start*/
#include  <stdlib.h>      /*exit*/
#include  <stdio.h>      /*exit*/
#include  "data_types.h"

void server_log(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(SERVER.log, fmt, ap);
  va_end(ap);
  fflush(SERVER.log);
}

void fatal(char *msg){
  perror(msg);
  exit(1);
}


