#include  "data_types.h"
#include  "logging.h"
#include  <stdarg.h>  /*va_start*/
#include  <stdlib.h>  /*exit*/
#include  <stdio.h>   /*exit*/
#include  <errno.h>   /*strerror*/
#include  <string.h>  /*strerror*/
#include  <time.h>   /*time*/


/* TODO: add time */
void server_log(int flag, char *fmt, ...) {
  char stime[32];

  struct tm *stm;
  time_t now = time(0);

  stm = localtime(&now);

  strftime (stime, sizeof(stime), "%Y-%m-%d %H:%M:%S", stm);

  /* begining, with date and time, ect */
  if ( flag == INFO ){
    fprintf(SERVER.log, "[%s] INFO: ", stime);
  }else if ( flag == ERROR ){
    fprintf(SERVER.log, "[%s] ERROR ", stime);
  }else if ( flag == FATAL ){
    fprintf(SERVER.log, "[%s] FATAL ", stime);
    if (SERVER.log != stderr)
      fprintf(stderr , "FATAL");
  }

  /* middle, what you are acutlaly suppose to say */
  va_list ap;
  va_start(ap, fmt);
  vfprintf(SERVER.log, fmt, ap);
  va_end(ap);

  /* errno end*/
  if ( flag & ( ERROR | FATAL && errno)){
      fprintf(SERVER.log, ":%s", strerror(errno));

    if ( SERVER.log != stderr && flag & FATAL )
      fprintf(stderr, ":%s", strerror(errno));
  }

  fprintf(SERVER.log, "\n");

  if ( SERVER.log != stderr && flag & FATAL )
    fprintf(stderr, ":%s", strerror(errno));


  fflush(SERVER.log);
  if ( flag & FATAL ) exit(errno ? errno : -1);

}
