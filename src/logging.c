#include  "data_types.h"
#include  "logging.h"
#include  "server.h"  
#include  <stdarg.h>  /*va_start*/
#include  <stdlib.h>  /*exit*/
#include  <stdio.h>   /*exit*/
#include  <errno.h>   /*strerror*/
#include  <string.h>  /*strerror*/
#include  <time.h>   /*time*/


/* TODO: add lock... because it writes in multiple stages */
void server_log(int flag, char *fmt, ...) {
  char stime[32];
  if (  ( flag & DEBUG )  &&  !DEBUG_ENABLED )
    return;

  struct tm *stm;
  time_t now = time(0);

  stm = localtime(&now);

  strftime (stime, sizeof(stime), "%Y-%m-%d %H:%M:%S", stm);

  /* begining, with date and time, ect */
  if ( flag == INFO ){
    fprintf(SERVER.log, "[%s] INFO: ", stime);
  }else if ( flag == DEBUG ){
    fprintf(SERVER.log, "[%s] DBUG: ", stime);
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

void hexdump(char *str){
  int i;
  int len = strlen(str);
  char buff1[3*16+1+5];
  char buff2[17];
  char *ptr = buff1;

  server_log(INFO, "HEXDUMP size: %d", len);
  
  for (i=0; i<len; i++){
    if ( i!=0 && i%16 == 0 ){
      server_log(INFO, "%s\t%s", buff1, buff2);
      buff1[0] = 0x00;
      ptr = buff1;
    }

    /* ascii stuff */
    if ( 
    ( str[i] >= (int) *"a" && str[i] <= (int) *"z" )  ||
    ( str[i] >= (int) *"A" && str[i] <= (int) *"Z" )  ||
    ( str[i] == (int) *"[" || str[i] == (int) *"]" )  ||
    ( str[i] >= (int) *"0" && str[i] <= (int)* "9" )  ){ 
      buff2[i%16] = str[i];
    }else{
      buff2[i%16] = *".";
    }
    buff2[(i%16)+1] = 0x00;

    sprintf(ptr, "%02x ", str[i] & 0xff);
    ptr+=3;

  }


  if ( i%16 != 0 )
      server_log(INFO, "%s\t%s", buff1, buff2);

}

