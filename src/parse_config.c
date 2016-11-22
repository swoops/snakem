#include <pwd.h> /*getpwnam_r*/
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include  "parse_config.h"
#include  "static_parse_config.h"
#include  "logging.h"
#include  "data_types.h"

#define MAX_CONFIG_LINE_LEN 1024


/* like strlen but stop on whitespace */
static size_t sp_strlen(char *buff){
  size_t ret;

  for (ret=0; buff[ret] != '\x00'; ret++)
    if  ( buff[ret] == '\t' ||  buff[ret] == ' ' ) 
      break;

  return ( ( buff[ret] & '\xff' ) != '\x00' ) ? ret : 0;
}

static char * skip_left_pad(char * buff){
    char * finger = buff;
		while ((*finger == ' ') || (*finger == '\t')) finger++;
    return finger;
}

/* 
 * change local user to a uid
 * thanks goes to Matt Joiner for the bassis of this. 
 * https://stackoverflow.com/questions/3836365/how-can-i-get-the-user-id-associated-with-a-login-on-linux
 * Was not my question but I drew from code he provided to clarify how the
 * struct would load the string buffers inside it and proper clean up
 */ 
static uid_t name_to_uid(char const *name) {
  if (!name) 
    server_log(FATAL, "%s:%d [name_to_uid] did not get a name" __FILE__, __LINE__);

  long const buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (buflen == -1) 
    server_log(FATAL, "%s:%d [name_to_uid] could not get _SC_GETPW_R_SIZE_MAX", __FILE__, __LINE__);

  char buf[buflen];
  struct passwd pwbuf, *pwbufp;

  /* from here on problems are about a line in the config, so we want to return
   * and output that line */
  if (getpwnam_r(name, &pwbuf, buf, buflen, &pwbufp) != 0){
    server_log(ERROR, "%s:%d [name_to_uid] Name \"%s\" not found on system", __FILE__, __LINE__, name);
    return -1;
  }

  /* errno should be set, we don't need to speculate what happened here */
  if ( !pwbufp ) return -1;
    
  return pwbufp->pw_uid;
}

static int set_params(char *buff, size_t buff_len, size_t line){
  size_t key_len;
  char * key = buff; /* for clairity, gcc should optimise this out :) */
  char * value;

  key_len = sp_strlen(key);
  if ( key_len == 0 ) return -1;
  server_log(DEBUG, "key: %s", key);
  key[key_len] = 0x00;

  value = skip_left_pad( &key[key_len+1] );

  if ( value[0] == '\x00' ){
    server_log(FATAL,"line %d does not have a value\n", line);
    return -1;
  }

  if       ( strcmp(key, "max_y") == 0 ){
		if ( (SERVER.max_y = atoi(value)) <= 0 )
      return 1;
  }else if ( strcmp(key, "max_x") == 0 ){
		if ( (SERVER.max_x = atoi(value)) <= 0 )
      return 1;
  }else if ( strcmp(key, "debug") == 0 ){
    if ( strcmp(value, "yes") == 0 )
      DEBUG_ENABLED = 1;
    else if ( strcmp(value, "no") == 0 )
      DEBUG_ENABLED = 0;
    else
      server_log(FATAL, "%s:%d [set_params] invalid debug value \"%s\" (must be yes/no)", __FILE__, __LINE__, value);

  /* local user to run server as */
  }else if ( strcmp(key, "user") == 0 ){
    if ( SERVER.uid  )
      server_log(FATAL, "Already set user for server, won't set it again config line: %zu", line);

    uid_t ret = name_to_uid(value);
    if ( ret == 0 )
      server_log(FATAL, "Server won't run under uid 0, username=\"%s\" line: %zu",
        value, line
      );
    else if ( ret < 0 )
      server_log(FATAL, "Could not get uid, username=\"%s\" line: %zu",
        value, line
      );
    else
      SERVER.uid = ret;
    server_log(DEBUG, "USER: %s UID: %i", value, SERVER.uid);
  }else if ( strcmp(key, "port") == 0 ){
    if ( (SERVER.port = atoi(value)) <= 0 ){
      server_log(DEBUG, "setting port to %d", SERVER.port);
      return 1;
    }
  }else if ( strcmp(key, "high_score") == 0 ){
		SERVER.high_score = atoi(value);
  }else if ( strcmp(key, "game_mods") == 0 ){
		SERVER.flags = atoi(value);
  }else if ( strcmp(key, "t_inc") == 0 ){
		if ( (SERVER.t_inc = atoi(value)) <= 0 )
      return 1;
  }else if ( strcmp(key, "max_players") == 0 ){
    /* this controls a size of a buffer, so be carefull */
    int v = atoi(value);

    if ( SERVER.max_players == v ) /* already done */
      return 0;

    /* new number, better make sure it is right */
		if ( v <= 0 )
      server_log(FATAL, "%s:%d [set_params] invalid number of players", __FILE__, __LINE__);

    SERVER.max_players = v;
    /* new number of players so realloc */
    SERVER.players = realloc(SERVER.players, sizeof(void *) * ( SERVER.max_players+1 ));
    if ( SERVER.players == NULL ) 
      server_log(FATAL, "%s:%d [set_params] realloc failed", __FILE__, __LINE__);
    else
      return 0;

  }else if ( strcmp(key, "log") == 0 ){
    if ( SERVER.log != stderr ) {
      SERVER.log = stderr;
      server_log(FATAL, "%s:%d [set_params] SERVER.log already set, line %zu", __FILE__, __LINE__, line);
    }
    if ( ( SERVER.log = fopen(value, "a") ) == NULL ){
      SERVER.log = stderr;
      server_log(FATAL, "%s:%d [set_params] could not open log file", __FILE__, __LINE__);
    }
  }else if ( strcmp(key, "start_banner") == 0 ){
    if ( SERVER.start_banner )
      server_log(FATAL, "%s:%d [set_params] start_banner already set, line: %zu", line);
    if ( (SERVER.start_banner = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for banner file name", __FILE__, __LINE__);
  }else if ( strcmp(key, "bot_warn") == 0 ){
    if ( SERVER.bot_warn  )
      server_log(FATAL, "%s:%d [set_params] bot_warn already set, line: %zu", line);
    if ( (SERVER.bot_warn = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for banner file name", __FILE__, __LINE__);
  }else if ( strcmp(key, "hs_name") == 0 ){
    if ( SERVER.hs_name == NULL )
      server_log(FATAL, "%s:%d [set_params] hs_name not set", __FILE__, __LINE__);
    else
      free(SERVER.hs_name);
    if ( (SERVER.hs_name = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for hs_name", __FILE__, __LINE__);
  }else if ( strcmp(key, "spec_name") == 0 ){
    if ( SERVER.spec_name )
      server_log(FATAL, "%s:%d [set_params] spec_name already set", __FILE__, __LINE__);
    if ( (SERVER.spec_name = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for spec_name", __FILE__, __LINE__);
  }else if ( strcmp(key, "spec_pass") == 0 ){
    if ( SERVER.spec_pass )
      server_log(FATAL, "%s:%d [set_params] spec_pass already set", __FILE__, __LINE__);
    if ( (SERVER.spec_pass = strdup(value) ) == NULL  )
      server_log(FATAL, "%s:%d [set_params] not enough memmory for spec_pass", __FILE__, __LINE__);
  }else{
    server_log(FATAL, "[set_params] line %d does not match any keys",	line);
  }
  return 0;

}
static void parse_add_str(char ***array, size_t *size, size_t max_size, char *value, int line){
  if ( *size == 0 ){
    /* first call was to realloc, nothing to do, return */
    if ( value == NULL )  
      server_log(FATAL, "%s:%d [parse_add_str] Last call was the first call, line: %d!!! ", __FILE__, __LINE__, line);

    *array = (char **) malloc(sizeof(char *) * max_size ); 
    if ( *array == NULL )
      server_log(FATAL, "%s:%d [parse_add_str] malloc failed, line %d:", __FILE__, __LINE__, line);

  }else if ( value == NULL ){
    /* should be last call */
    if ( ( *array = realloc(*array, sizeof(char *) * (*size)) ) == NULL )
      server_log(FATAL, "%s:%d [parse_add_str] realloc failed, line: %d!!! ", __FILE__, __LINE__, line);
    return ;
  }

  if ( *size >= max_size )
    server_log(FATAL, "%s:%d [parse_add_str] too many values, line %d:", __FILE__, __LINE__, line);


  char * ptr;
  ptr = strdup(value);
  if ( ptr == NULL )
    server_log(FATAL, "%s:%d [parse_add_str] could not get memory for value \"%s\", line %d:", __FILE__, __LINE__, value, line);

   (*array)[(*size)++] = ptr; 
}

static void iter_param_list(FILE *fp, char *name, int *line){
  char buff[MAX_CONFIG_LINE_LEN];
  char *** key;
  size_t *size;
  size_t max_size;
  long pos = ftell(fp);
  char * finger;
  size_t buff_len;
  unsigned int sorted = 0;

  if ( strcmp(name, "bad_names") == 0 ){
    key =  &SERVER.bnames;
    size = &SERVER.num_bnames;
    max_size = 128; 
    sorted = 1;
  } else{
    server_log(FATAL, "Unreconised list name %s on line %zu", name, *line);
  }

  /* 
   * TODO: next line is pretty much seen twice, make it seen once in a nice
   * function :) 
  */
	while (!feof(fp) && ( fgets(buff, sizeof(buff), fp) != (char*) NULL) && (!ferror(fp))) {
    finger = skip_left_pad(buff);

    /* skip blank lines */
		if ((*finger == '\r') || (*finger == '\n') || (*finger == '#')) 
      continue;

    /* line does not start with space, so end the list */
    if ( finger == buff ) break;

    /* continuing in list, so update info */
    *line = *line + 1;
    pos = ftell(fp);

    if ( (buff_len = strlen(buff) ) == sizeof(buff) - 1 )
      server_log(FATAL,"%s:%d [parse_file] line %d too long", __FILE__, __LINE__, *line);

    if ( buff[buff_len-1] == '\n' )
      buff[--buff_len] = '\x00';
    else
      server_log(FATAL, "%s:%d [parse_file] line %d did not find a new line", __FILE__, __LINE__, *line);

    parse_add_str(key, size, max_size, finger, *line);
  }

  /* finished up, clean things up */
  parse_add_str(key, size, max_size,  NULL, *line);
  fseek(fp, pos, SEEK_SET);

  /* sort the array alphabeticly? */
  if ( *size == 0 || sorted == 0 ) 
    return;

  /* scratch buffer for sorting */
  char **ptr;
  if ( ( ptr = calloc(*size, sizeof(char *)) ) == NULL ) 
    server_log(FATAL, 
      "%s:%d [parse_file] calloc returned NULL",
      __FILE__, __LINE__
    );

  if ( ptr == merge_sort_str(*key, ptr, *size) ){
    free(*key);
    *key = ptr;
  }else{
    free(ptr);
  }
}

static char ** merge_sort_str(char **old, char **new, size_t size){
  /* always switch two buffers */
  if ( size == 1 ) {
    new[0] = old[0];
    return new;
  }

  size_t lsize =  size/2;
  size_t rsize = size - lsize;

  char **left  = merge_sort_str(old, new, lsize);
  char **right = merge_sort_str(old+lsize, new+lsize, rsize);
  char **ret = (left == old) ? new : old;

  size_t i; 
  size_t li=0,ri=0;
  for (i=0; i<size; i++) /* rsize should be bigger, check the assert above */
    if ( li == lsize || ( ri != rsize &&  strcmp(right[ri], left[li]) < 0 ))
      ret[i] = right[ri++];
    else
      ret[i] = left[li++];

  return ret;

}

int parse_file(char *fname){
  FILE * fp; 
  char buff[MAX_CONFIG_LINE_LEN];
  size_t buff_len;
  int line = 0;
  char *finger; /* points to where you are on the line */

  /* return code, 0 => good */
  int ret = 0; 

  if ( (fp = fopen(fname, "r")) == NULL )
    server_log(FATAL, "[parse_file] file open failed: ");
  
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
    if ( finger != buff )
      server_log(FATAL,"%s:%d [parse_file] line %zu, char %zu: starts with  white space \"%s\"", __FILE__, __LINE__,  line, (size_t) (finger - buff), buff);

    if ( (buff_len = strlen(buff) ) == sizeof(buff) - 1 )
      server_log(FATAL,"[parse_file] line %zu too long", line);

    /* remove new line char */
    if ( buff[buff_len-1] == '\n' )
      buff[--buff_len] = '\x00';
    else
      server_log(FATAL, "[parse_file] line %zu did not find a new line", line);

    if ( strncmp(buff, "list",4) == 0  ){
      iter_param_list(fp, skip_left_pad( buff+4), &line);
    }else  if ( set_params(buff, buff_len, line) != 0 ){
      server_log(FATAL, "[parse_file] line %zu set_params return non zero value", line);
    }
  }
  fclose(fp);
  return ret;
}
