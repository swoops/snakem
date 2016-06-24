#include  <string.h>      /*strlen*/
#include  <stdio.h>       /*strlen*/
#include  <stdlib.h>
#include  "data_types.h"
#include  "movement.h"
#include  "player.h"

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
