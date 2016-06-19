#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "data_types.h"
#include "movement.h"
#include <sys/socket.h>

void go_home_cursor(){
  fprintf(FDOUT, "\e[H");
}

// 0,0 is home... or should it be?
void move_cursor_abs(int x, int y){
  x = check_bounds(x, SERVER.max_x, 0);
  y = check_bounds(y, SERVER.max_y, 0);

  /*
   * go home 0,0 then move down and right x,y would be better to move relative
   * to current location but swoops that for now...
  */
  go_home_cursor();
  if ( x > 0 )
    fprintf(FDOUT, "\e[%dC", x);
  if ( y > 0 )
    fprintf(FDOUT, "\e[%dB", y);
}

void place_str(int x, int y, char *fmt, ...) {
  move_cursor_abs(x,y);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(FDOUT, fmt, ap);
  va_end(ap);
  go_home_cursor();
}
int check_bounds(int n, int max, int min){
  if ( n <= min )
    return min;
  else if ( n >= max )
    return max;
  else
    return n;
}

int cord_to_num(point *p){
  // top left corner of play board is at (3,3)
  short int ret;

  // no good
  if ( p->x < 3 || p->y < 3 ) return 0;
  if ( p->x > SERVER.max_x - 3 || p->y > SERVER.max_y - 2 ) return 0;

  return ( p->y-3 ) * ( SERVER.max_x - 5 ) + p->x-2;
}

int num_to_cord(int num, point *p){
  // no good
  if ( num < 1 || num > (SERVER.max_x - 3)*(SERVER.max_y-2 ) ) return 0;

  p->x = num % (SERVER.max_x - 5);
  if ( p->x == 0 ) p->x += SERVER.max_x - 5;
  p->y = (( num - p->x ) / (SERVER.max_x - 5)) + 3;
  p->x = p->x + 2;

  return 1;
}

