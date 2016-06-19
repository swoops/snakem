#include<stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include <unistd.h> /*usleep*/
#include <string.h>
#include "data_types.h"
#include "movement.h"
#include "menu.h"
#include "debug.h"
#include "snake.h"
#include  "player.h"

void draw_snake(player *p){
  int i;
  point pos;

  for ( i=0; i<p->slen; i++){
    if ( num_to_cord(p->pix[i], &pos) != 1)
      debug(DEBUG_PAUSE, "[draw_snake]: num_to_cord(%d, head) != 1", p->pix[i]);
    place_str(pos.x, pos.y, "s");
  }
}

void change_dir(player *p, unsigned  int dir){
    if ( p->dir & HOLDD ) return;
    if ( p->dir & (LEFT | RIGHT ) && dir & (LEFT | RIGHT )  ) return;
    if ( p->dir & (UP | DOWN ) && dir & (UP | DOWN )  ) return;

    /* TODO: unlock/lock wrapper to check for player==NULL */
    player_lock(&p->lock);
    p->dir = dir | HOLDD;
    player_unlock(&p->lock);
}

int player_controll(player *p){
  char ch;

  while(1){
    /* get character */
    read(p->fd, &ch, 1);
    if ( ch == 'q' ) break;

    switch(ch){
        case 'l':
        case 'd':
          change_dir(p, RIGHT);
          break;
        case 'h':
        case 'a':
          change_dir(p, LEFT);
          break;
        case 'k':
        case 'w':
          change_dir(p, UP);
          break;
        case 'j':
        case 's':
          change_dir(p, DOWN);
          break;
        case ' ':
          /* to pause just lock until done */
          /* TODO: disable for two players */
          player_lock(&p->lock);
					debug(0, "\e[%dCPAUSED", (SERVER.max_x - 6)/2);
					while(fgetc(stdin) != ' ')
						usleep(1000);
					debug(0, "\e[0m\e[2K");
          player_unlock(&p->lock);

          break;
    }
  }
  if ( p->fd )
    write(p->fd, CLEAR_SCREEN, sizeof(CLEAR_SCREEN));
  else
    printf(CLEAR_SCREEN);
  go_home_cursor();
  return 0;

}

int progess_game(player *p){
  p->delay =  100000; 
  draw_snake(p);
  put_pellet(p);
  show_score(p->score);
  while(1){
    usleep(p->delay);
    move_snake(p);
    fflush(FDOUT);
  }
}

void grow_snake(player *p){
  p->score += 100;
  show_score(p->score);
  if ( p->size == p->slen )
    winner();  // TODO: realloc memory so no one wins... ever...
  int i;
  for ( i=p->slen++; i >= p->head; i-- )
    p->pix[i+1] = p->pix[i];

  p->delay -= SERVER.t_inc;  // go faster

  p->head = ( p->head+1 ) % p->slen;
  // add anything that is not divisible by 2 
  // euler ftw :)
  p->color = ( p->color + 13 ) % 256; 
}
void move_snake(player *p){
  point phead, ptail;
  int head_num;

  int taili = p->head-1;
  if ( taili < 0 ) taili = taili + p->slen;

  if (  num_to_cord(p->pix[p->head], &phead) != 1 )
    debug(DEBUG_PAUSE, "[move_snake]: num_to_cord(%d, head) != 1", p->pix[p->head]);

  if (  num_to_cord(p->pix[taili], &ptail) != 1 )
    debug(DEBUG_PAUSE, "[move_snake]: num_to_cord(%d, head) != 1", p->pix[taili]);


  // find new head
  player_lock(&p->lock);
  // unset hold direction flag if there...
  if ( p->dir & HOLDD )
    p->dir = p->dir ^ HOLDD;  
  switch (p->dir){
    case UP:
      phead.y = phead.y - 1;
      break;
    case DOWN:
      phead.y = phead.y + 1;
      break;
    case RIGHT:
      phead.x = phead.x + 1;
      break;
    case LEFT:
      phead.x = phead.x - 1;
      break;
  }
  player_unlock(&p->lock);

  // get new position of head
  head_num = cord_to_num(&phead);

  // hit wall?
  if ( !head_num ) game_over();


  /*
   * check pellet before "expensive" check of hitting itself
  */
  if ( head_num != p->pellet ){
    /* check hit tail */
    int i;
    for ( i=1; i<p->slen; i++ )
      if ( p->pix[i] == head_num )
        game_over();

    /* collisions done, remove old tail because no pellot */
    place_str( ptail.x, ptail.y, "\e[48;5;%dm \e[0m", p->color);
    /* place_str( ptail.x, ptail.y, " "); */

  }else{
    grow_snake(p);
    put_pellet(p);
    taili = p->head-1;
    if ( taili < 0 ) taili = taili + p->slen;
  }

  place_str( phead.x, phead.y, "S");
  /* place_str( phead.x, phead.y, "\e[48;5;%dm$\e[0m", ( p->color+5 ) % 256); */

  /* update memory struct */
  p->head = taili;
  p->pix[p->head] =  head_num;
  /* debug(0, "[move_snake]  head (%d, %d): %d", phead.x, phead.y, p->pix[p->head]); */

}
void put_pellet(player *p){
  point pellet;

  // TODO: not this probably...
  // squre is from (3,3) to (max_x-3, max_y-2)
  int num = random() % ( ( SERVER.max_y-4 ) * (SERVER.max_x-5 ) );

  if ( num_to_cord(num, &pellet)  == 0 ){
    debug(DEBUG_PAUSE, "[put_pellet]: pellet out of bounds (%d, %d): %d", pellet.x, pellet.y, num);
  }
  p->pellet = num;
  place_str( pellet.x, pellet.y, "\e[38;5;0;48;5;46m*\e[0m");
  /* place_str( pellet.x, pellet.y, "\e[38;5;46m*\e[0m"); */
}
