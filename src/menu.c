#include  <string.h>      /*strlen*/
#include  <stdio.h>       /*strlen*/
#include  <stdlib.h>
#include  "data_types.h"
#include  "movement.h"
#include  "player.h"
#include  "logging.h"
#include  "server.h"

void show_score(player *p){
  char buff[128];

  serv_lock();
  if ( SERVER.hs_name == NULL )
    server_log(FATAL, "%s [show_score]  line:%d should not have NULL highscore name", __FILE__, __LINE__ );

  snprintf(buff, sizeof(buff), 
      "\e[H\e[2K Your Score\e[%dCHIGH SCORE\e[H\e[1B"
      "\e[2K%8u\e[%zuC%s:%8u",
      SERVER.max_x - 22,
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
  char bar[SERVER.max_x ];
  memset(bar, 0x23, SERVER.max_x-1 );
  bar[SERVER.max_x-1] = 0x00; /* just to be sure */

  /* draaw top of box */
  place_str( 1, 2, p, bar);
  /* place edges */
  int i;
  for ( i=3; i<SERVER.max_y-1; i++ ){
    place_str( 1, i, p, "#\e[%dC#", SERVER.max_x-3);
  }
  /* place bottom */
  place_str( 1, SERVER.max_y-1, p,  bar);

  go_home_cursor(p);
}
