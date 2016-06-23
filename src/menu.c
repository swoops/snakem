#include  <string.h>      /*strlen*/
#include  <stdio.h>       /*strlen*/
#include  <stdlib.h>
#include  "data_types.h"
#include  "movement.h"

void show_score(unsigned int score){
  place_str(0, 1, "SCORE: %10u", score);
}

void game_over(){
  char str[] = "GAME OVER!!!";
  int x,y,i;

  x = (SERVER.max_x - strlen(str))/2;
  y = ( SERVER.max_y/2 ) - 1;

  for ( i=0; i<3; i++ ){
    place_str( x, y++, "%s", str);
		if (SERVER.port == 0) fflush(FDOUT);
    usleep(500000);
  }
  fprintf(FDOUT, "\e[2J");
  go_home_cursor();
}

void winner(){
  char str[] = "A WINNDER IS YOU!!!";
  int x,y,i;

  x = (SERVER.max_x - strlen(str))/2;
  y = ( SERVER.max_y/2 ) - 1;

  for ( i=0; i<3; i++ ){
    place_str( x, y++, "%s", str);
    if ( SERVER.port == 0  ) fflush(FDOUT);
    usleep(500000);
  }

  fprintf(FDOUT, "\e[2J");
  go_home_cursor();
}

void draw_board(){
  char bar[SERVER.max_x ];
  memset(bar, 0x23, SERVER.max_x-1 );
  bar[SERVER.max_x-1] = 0x00; /* just to be sure */

  /* draaw top of box */
  place_str( 1, 2, bar);
  /* place edges */
  int i;
  for ( i=3; i<SERVER.max_y-1; i++ ){
    place_str( 1, i, "#\e[%dC#", SERVER.max_x-3);
  }
  /* place bottom */
  place_str( 1, SERVER.max_y-1, bar);

  go_home_cursor();
}
