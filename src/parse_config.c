#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include  "logging.h"
#include  "data_types.h"

#define MAX_CONFIG_LINE_LEN 1024

/* like strlen but stop on whitespace */
size_t sp_strlen(char *buff){
  size_t ret;

  for (ret=0; buff[ret] != '\x00'; ret++)
    if  ( buff[ret] == '\t' ||  buff[ret] == ' ' ) 
      break;

  return ( ( buff[ret] & '\xff' ) != '\x00' ) ? ret : 0;
}

char * skip_left_pad(char * buff){
    char * finger = buff;
		while ((*finger == ' ') || (*finger == '\t')) finger++;
    return finger;
}

int set_params(char *buff, size_t buff_len, size_t line){
  size_t key_len;
  char * key = buff; /* for clairity, gcc should optimise this out :) */
  char * value;

  key_len = sp_strlen(key);
  if ( key_len == 0 ) return -1;

  value = skip_left_pad( &key[key_len+1] );

  if ( value[0] == '\x00' ){
    server_log(FATAL,"line %d does not have a value\n", line);
    return -1;
  }

  if       ( strncmp(key, "max_y",        key_len) == 0 ){
		SERVER.max_y = atoi(value);
  }else if ( strncmp(key, "max_x",        key_len) == 0 ){
		SERVER.max_x = atoi(value);
  }else if ( strncmp(key, "debug",        key_len) == 0 ){
    if ( strcmp(value, "yes") == 0 )
      DEBUG_ENABLED = 1;
    else if ( strcmp(value, "no") == 0 )
      DEBUG_ENABLED = 0;
    else
      server_log(FATAL, "%s:%d [set_params] invalid debug value (must be yes/no)" __FILE__, __LINE__);
  }else if ( strncmp(key, "port",         key_len) == 0 ){
		SERVER.port = atoi(value);
  }else if ( strncmp(key, "high_score",   key_len) == 0 ){
		SERVER.high_score = atoi(value);
  }else if ( strncmp(key, "t_inc",        key_len) == 0 ){
		SERVER.t_inc = atoi(value);
  }else if ( strncmp(key, "max_players",  key_len) == 0 ){
		SERVER.max_players = atoi(value);
    if ( SERVER.max_players <= 0 )
      server_log(FATAL, "%s:%d [set_params] invalid number of players" __FILE__, __LINE__);
  }else if ( strncmp(key, "log",          key_len) == 0 ){
    if ( SERVER.log != stderr ) {
      SERVER.log = stderr;
      server_log(FATAL, "%s:%d [set_params] SERVER.log != NULL" __FILE__, __LINE__);
    }
    if ( ( SERVER.log = fopen(value, "a") ) == NULL ){
      SERVER.log = stderr;
      server_log(FATAL, "%s:%d [set_params] could not open log file", __FILE__, __LINE__);
    }
  }else if ( strncmp(key, "start_banner", key_len) == 0 ){
    if ( (SERVER.start_banner = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for banner file name", __FILE__, __LINE__);
  }else if ( strncmp(key, "bot_warn",     key_len) == 0 ){
    if ( (SERVER.bot_warn = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for banner file name", __FILE__, __LINE__);
  }else if ( strncmp(key, "hs_name",  key_len) == 0 ){
    if ( (SERVER.hs_name = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for hs_name", __FILE__, __LINE__);
  }else if ( strncmp(key, "spec_name",  key_len) == 0 ){
    if ( (SERVER.spec_name = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for spec_name", __FILE__, __LINE__);
  }else if ( strncmp(key, "spec_pass",  key_len) == 0 ){
    if ( (SERVER.spec_pass = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for spec_pass", __FILE__, __LINE__);
  /* TODO: parse flags value... */
  /*
  }else if ( strncmp(key, "flags",  key_len) == 0 ){
    printf("line %d flags: \"%s\"\n", 	line, value);
  */
  }else{
    server_log(FATAL, "[set_params] line %d does not match any keys",	line);
  }

  return 0;

}

int parse_file(char *fname){
  FILE * fp; 
  char buff[MAX_CONFIG_LINE_LEN];
  size_t buff_len, finger_len;
  int line = 0;
  char *finger; /* points to where you are on the line */

  /* return code, 0 => good */
  int ret = 0; 

  if ( (fp = fopen(fname, "r")) == NULL ){
    perror("file open failed: ");
    return -1;
  }
  
  /* 
   * much of the following is inspired (or maybe stolen?) from, the FreeRadIUS
   * pam module found here:
   * https://github.com/FreeRADIUS/pam_radius/blob/master/src/pam_radius_auth.c#L617-L671
   * Thanks alandekok
  */
	while (!feof(fp) && ( fgets(buff, sizeof(buff), fp) != (char*) NULL) && (!ferror(fp))) {
		line++;

    finger = skip_left_pad(buff);

		/*
		 *	Skip blank lines and comments.
		 */
		if ((*finger == '\r') || (*finger == '\n') || (*finger == '#')) continue;

    if ( (buff_len = strlen(buff) ) == sizeof(buff) - 1 ){
      server_log(FATAL,"[parse_file] line %zu too long", line);
      ret =  -2;
      break;
    }

    /* remove new line char */
    if ( buff[buff_len-1] == '\n' )
      buff[--buff_len] = '\x00';

    finger_len = buff_len - ( finger - buff );

    if ( set_params(finger, finger_len, line) != 0 ){
      ret = -1;
      break;
    }
  }
  fclose(fp);
  return ret;
}
