#include<stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include <unistd.h> /*usleep*/
#include <string.h>
#include "data_types.h"
#include "movement.h"
#include "menu.h"
#include "snake.h"
#include  "player.h"
#include  "logging.h"


void draw_snake(player *p){
  int i;
  point pos;

  for ( i=0; i<p->slen; i++){
    if ( num_to_cord(p->pix[i], &pos) != 1)
      server_log(ERROR, "[draw_snake]: num_to_cord(%d, head) != 1", p->pix[i]);
    place_str(pos.x, pos.y, p, "s");
  }
}

void change_dir(player *p, unsigned  int dir){
    if ( p->dir & HOLDD ) return;
    if ( p->dir & (LEFT | RIGHT ) && dir & (LEFT | RIGHT )  ) return;
    if ( p->dir & (UP | DOWN ) && dir & (UP | DOWN )  ) return;

    player_lock(p);
    p->dir = dir | HOLDD;
    player_unlock(p);
}

int player_controll(player *p){
  char ch;

  while((ch = pgetc(p)) != 'q' ){
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
          player_lock(p);
					while(pgetc(p) != ' ')
						usleep(1000);
          player_unlock(p);
          break;
        default:
          player_lock(p);
          player_unlock(p);
    }
  }
  player_write(p, CLEAR_SCREEN);
  destroy_player(p);
  return 0;

}

int progess_game(player *p){
  p->delay =  100000; 

  if ( p->fd ){
    /* 
     * tell tellnet not to write characters to the screen, send every keypress,
     * and don't be such a jerk... thanks SO:
     * https://stackoverflow.com/questions/4532344/send-data-over-telnet-without-pressing-enter
    */
    player_write(p, 
      "\xff\xfd\x22" /* IAC DO LINEMODE*/
      "\xff\xfb\x01" /* IAC WILL ECHO */
    ); 
    player_write(p,CLEAR_SCREEN); 
		if ( SERVER.start_banner  && ! write_file(SERVER.start_banner , p) )
				usleep(5000000);
  }
  player_write(p,CLEAR_SCREEN); 

  draw_board(p);
  draw_snake(p);
  put_pellet(p);
  show_score(p);
  while(1){
    usleep(p->delay);
    move_snake(p);
    if (SERVER.port == 0 ) fflush(FDOUT);
  }
}

void grow_snake(player *p){
  p->score += 50;
  show_score(p);
  if ( p->size == p->slen ){
    winner(p);  // TODO: realloc memory so no one wins... ever...
    destroy_player(p);
  }
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
    server_log(ERROR, "[move_snake]: num_to_cord(%d, head) != 1", p->pix[p->head]);

  if (  num_to_cord(p->pix[taili], &ptail) != 1 )
    server_log(ERROR, "[move_snake]: num_to_cord(%d, head) != 1", p->pix[taili]);


  // find new head
  player_lock(p);
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
  player_unlock(p);

  // get new position of head
  head_num = cord_to_num(&phead);

  // hit wall?
  if ( !head_num ) {
    game_over(p);
    destroy_player(p);
  }


  /*
   * check pellet before "expensive" check of hitting itself
  */
  if ( head_num != p->pellet ){
    if ( snake_collision(p, head_num) ){
      game_over(p);
      destroy_player(p);
    }

    /* collisions done, remove old tail because no pellot */
    place_str( ptail.x, ptail.y, p,  "\e[48;5;%dm \e[0m", p->color);
  }else{
    grow_snake(p);
    put_pellet(p);
    taili = p->head-1;
    if ( taili < 0 ) taili = taili + p->slen;
  }

  place_str( phead.x, phead.y, p, "S");

  /* update memory struct */
  p->head = taili;
  p->pix[p->head] =  head_num;

}

/* TODO: skip taxi cab distance through array? */
int snake_collision(player *p, int col){
  int i;

  for ( i=1; i<p->slen; i++ )
    if ( p->pix[i] == col )
      return 1;

  return 0;
}

void put_pellet(player *p){
  point pellet;
  int max_num = (SERVER.max_x - 3)*(SERVER.max_y-2 ); 

  // squre is from (3,3) to (max_x-3, max_y-2)
  int num = random() % ( ( SERVER.max_y-4 ) * (SERVER.max_x-5 ) );

  /* 
   * ensure the pellot is not in the snake 
   * TODO: there has to be a better way!
  */

  /*
    not so simple...
    while ( snake_collision(p, num) )
      num = (num + 1) % max_num;
  */

  if ( num_to_cord(num, &pellet)  == 0 ){
    server_log(ERROR, "[put_pellet]: pellet out of bounds (%d, %d): %d", pellet.x, pellet.y, num);
  }
  p->pellet = num;
  place_str( pellet.x, pellet.y, p, "\e[38;5;0;48;5;46m*\e[0m");
}
