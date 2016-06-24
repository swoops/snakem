#include  "data_types.h"
#include  "logging.h"
#include  <stdarg.h>  /*va_start*/
#include  <stdlib.h>  /*exit*/
#include  <stdio.h>   /*exit*/
#include  <errno.h>   /*sterror*/
#include  <string.h>  /*sterror*/
#include  <time.h>   /*time*/


/* TODO: add time */
void server_log(int flag, char *fmt, ...) {
  char stime[32];
  char buff[1024];

  struct tm *stm;
  time_t now = time(0);
  stm = localtime(&now);

  strftime (stime, sizeof(stime), "%Y-%m-%d %H:%M:%S", stm);


  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buff, sizeof(buff), fmt, ap);
  va_end(ap);

  if ( flag == INFO ){
    fprintf(SERVER.log, "[%s] INFO: %s\n", stime, buff);
  }else if ( flag == ERROR ){
    fprintf(SERVER.log, "[%s] ERROR: %s\n", stime, buff);
  }else if ( flag == FATAL ){
    fprintf(SERVER.log, "[%s] FATAL: %s: %s\n", stime, buff, strerror(errno));
    exit(1);
  }

  fflush(SERVER.log);

}
