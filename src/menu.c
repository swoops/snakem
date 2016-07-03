#include  <string.h>      /*strlen*/
#include  <stdio.h>       
#include  <stdlib.h>
#include  "data_types.h"
#include  "movement.h"
#include  "player.h"
#include  "logging.h"
#include  "server.h"

void show_score(player *p){
  char buff[40+MAX_PLAYER_NAME];

  serv_lock();
  if ( SERVER.hs_name == NULL )
    server_log(FATAL, "%s [show_score]  line:%d should not have NULL highscore name", __FILE__, __LINE__ );

  snprintf(buff, sizeof(buff), 
      "\e[H\e[1B\e[2K%8u\e[%zuC%s:%8u\e[H",
      p->score, 
      (size_t) SERVER.max_x - 17 - strlen(SERVER.hs_name), 
      SERVER.hs_name, SERVER.high_score
  );
  serv_unlock();
  player_write(p, buff);
}

void game_over(player *p){
  char str[] = "GAME OVER!!!";
  int x,y,i;

  x = (SERVER.max_x - strlen(str))/2;
  y = ( SERVER.max_y/2 ) - 1;

  for ( i=0; i<3; i++ ){
    place_str( x, y++, p, "%s", str);
		if (SERVER.port == 0) fflush(FDOUT);
    usleep(500000);
  }
  clear_screen(p);
  go_home_cursor(p);
}

int write_file(char *fname, player *p){
  int bytes_read;
  void *bookmark;
  int bytes_written = 0;
  char buff[256];

  FILE *fp; 
  if (  ( fp = fopen(fname, "r") ) == NULL){
      server_log(ERROR, "[write_file] could not open file: \"%s\"", fname);
      return 1;
  }

  while ( ( bytes_read = fread(buff, sizeof(char),  sizeof(buff), fp) ) != 0  ) {

    if (bytes_read < 0) {
      server_log(ERROR, "[write_file] Failed to read from file %s", fname);
      fclose(fp);
      return 1;
    }

    bookmark = buff;
    while (bytes_read > 0) {
      bytes_written = write(p->fd, bookmark, bytes_read);
      if (bytes_written <= 0) {
        server_log(ERROR, "[write_file] could not write to socket");
        fclose(fp);
        destroy_player(p);
      }
      bytes_read -= bytes_written;
      bookmark += bytes_written;
    }

  }
  fclose(fp);
  return 0;

}

void winner(player *p){
  char str[] = "A WINNDER IS YOU!!!";
  int x,y,i;

  x = (SERVER.max_x - strlen(str))/2;
  y = ( SERVER.max_y/2 ) - 1;

  for ( i=0; i<3; i++ ){
    place_str( x, y++, p, "%s", str);
    if ( SERVER.port == 0  ) fflush(FDOUT);
    usleep(500000);
  }

  clear_screen(p);
  go_home_cursor(p);
}

void draw_board(player *p){
  #define TMP_STR1 "\e[H\e[2K Your Score\e["
  #define TMP_STR2 "CHIGH SCORE\e[H\e[2B "
  int i;
  int s_buff  = sizeof(TMP_STR1) + 3 + sizeof(TMP_STR2)  /* SCIRE: string + shift right ammount + string + home+down2 */
              + SERVER.max_x - 1												 /* first bar (easy) */
						  + (SERVER.max_y-4) * 18									   /* sides: max_len  * height */      
						  + 10																			 /* one more newline backup */
              + SERVER.max_x - 1												 /* second bar (easy) */
						  + 1;																			 /* Null terminate */

  char buff[s_buff];
  char *ptr;

  /* SCORE PART */
  sprintf( buff,
      TMP_STR1 "%d" TMP_STR2,
      SERVER.max_x - 22
  );

  ptr = buff + strlen(buff);

  /* add top bar */
  memset(ptr, 0x23, SERVER.max_x-1 );
  ptr += SERVER.max_x-1;

  for ( i=0; i<SERVER.max_y-4; i++ ){
    ptr += sprintf( ptr,
      "\e[1B\e[%dD#\e[%dC#",  /*max len is 18 */
			SERVER.max_x-1,
			SERVER.max_x-3
    );
  }
	/* new line and go back one more time */
	sprintf( ptr,
		"\e[1B\e[%dD",				/* maxlen 10 */
		SERVER.max_x-1
	);

  ptr = buff + strlen(buff);

	/* bottom bar */
  memset(ptr, 0x23, SERVER.max_x-1 );
  ptr += SERVER.max_x-1;

	/* null terminate */
  ptr[0] = 0x00;

  /* a little bit of "sanity" to go with this mess :) */
	if (s_buff < (unsigned int) (ptr-buff))
    server_log(FATAL, "%s [draw_board]  line:%d detected overflow!!!", __FILE__, __LINE__ );

  write(p->fd, buff, (ssize_t) (ptr-buff));

  #undef TMP_STR1
  #undef TMP_STR2
}
