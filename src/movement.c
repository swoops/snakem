#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "data_types.h"
#include "movement.h"
#include "player.h"
#include "server.h"
#include <sys/socket.h>

void clear_screen(player *p){
  if (p != NULL)
    write(p->fd, CLEAR_SCREEN_STR, sizeof(CLEAR_SCREEN_STR));
  else
    serv_write(CLEAR_SCREEN_STR);
}

void go_home_cursor(player *p){
  if (p != NULL)
    write(p->fd, GO_HOME_STR, sizeof(GO_HOME_STR));
  else
    serv_write(GO_HOME_STR);
}

void place_str(int x, int y, player *p, char *fmt, ...) {
  char buff[MAX_BUFF_SIZE];
  char *ptr = buff;
  char move[] = "\e[%d;%df";
  int move_len = sizeof(move)+6;  /* d is at most 3 chars long */

  if ( x>=0 && y>=0 )
    ptr+=sprintf(buff, move, y+1, x+1);

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(ptr, sizeof(buff)-move_len, fmt, ap);
  va_end(ap);

  strncat(buff, GO_HOME_STR, sizeof(buff));

  if (p != NULL)
    player_write(p,buff);
  else
    serv_write(buff);
  go_home_cursor(p);
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

