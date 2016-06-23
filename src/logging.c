#include  "data_types.h"
#include  <stdarg.h>  /*va_start*/
#include  <stdlib.h>  /*exit*/
#include  <stdio.h>   /*exit*/
#include  <errno.h>   /*sterror*/
#include  <string.h>  /*sterror*/

/* TODO: add time */
void server_log(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(SERVER.log, fmt, ap);
  va_end(ap);
  fflush(SERVER.log);
}

void fatal(char *msg){
  server_log("[!] Fatal! %s: %s\n", msg, strerror(errno));
  perror(msg);
  exit(1);
}


