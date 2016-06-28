#include  <string.h>      /*strlen*/
#include  <stdio.h>       /*strlen*/
#include  <stdlib.h>
#include  "data_types.h"
#include  "movement.h"
#include  "player.h"
#include  "logging.h"

void show_score(player *p){
  int offset = SERVER.max_x - 35;
  if ( SERVER.high_score  && offset > 0)
    place_str(0, 1,p, "SCORE: %8u\e[%dCHIGH SCORE: %8u", p->score, offset,  SERVER.high_score);
  else
    place_str(0, 1,p, "SCORE: %8u", p->score);
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
  player_write(p, "\e[2J");
  go_home_cursor(p);
}

int write_file(char *fname, player *p){
  int bytes_read;
  void *bookmark;
  int bytes_written = 0;
  char buff[256];

  FILE *fp; 
  server_log(ERROR, "[write_file] p at %p", p);
  if (  ( fp = fopen(fname, "r") ) == NULL){
      server_log(ERROR, "[write_file] could not open file: \"%s\"", fname);
      return 1;
  }

  while ( ( bytes_read = fread(buff, sizeof(char),  sizeof(buff), fp) ) != 0  ) {

    server_log(INFO, "buff has %d bytes\n", bytes_read);
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
        return 1;
      }
      server_log(INFO, "write: %d bytes to %d\n", bytes_written, p->fd);
      server_log(ERROR, "[write_file] p at %p", p);
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

  player_write(p, "\e[2J");
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
