#include  <string.h>      /*strlen*/
#include  <stdio.h>       /*strlen*/
#include  <stdarg.h>      /*va_start*/
#include  <stdlib.h>
#include  "data_types.h"
#include  "movement.h"

void show_score(unsigned int score){
  place_str(0, 1, "SCORE: %10u", score);
}

void server_log(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(SERVER.log, fmt, ap);
  va_end(ap);
  fflush(SERVER.log);
}

void fatal(char *msg){
  perror(msg);
  exit(1);
}


void game_over(){
  char str[] = "GAME OVER!!!";
  int x,y,i;

  x = (SERVER.max_x - strlen(str))/2;
  y = ( SERVER.max_y/2 ) - 1;

  for ( i=0; i<3; i++ ){
    place_str( x, y++, "%s", str);
    fflush(FDOUT);
    usleep(500000);
  }
  fgetc(stdin);
  fprintf(FDOUT, "\e[2J");
  go_home_cursor();
  exit(0);
}

void winner(){
  char str[] = "A WINNDER IS YOU!!!";
  int x,y,i;

  x = (SERVER.max_x - strlen(str))/2;
  y = ( SERVER.max_y/2 ) - 1;

  for ( i=0; i<3; i++ ){
    place_str( x, y++, "%s", str);
    fflush(FDOUT);
    usleep(500000);
  }

  fgetc(stdin);
  fprintf(FDOUT, "\e[2J");
  go_home_cursor();
  exit(0);
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
