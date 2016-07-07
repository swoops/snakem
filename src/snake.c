#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> /*usleep*/
#include <string.h>
#include "data_types.h"
#include "movement.h"
#include "menu.h"
#include "snake.h"
#include "server.h"
#include "player.h"
#include "logging.h"

#define CAT_CODE  "\xf0\x9f\x90\xb1"


void draw_snake(player *p){
  int i;
  point pos;

  for ( i=0; i<p->slen; i++){
    if ( num_to_cord(p->pix[i], &pos) != 1)
      server_log(ERROR, "[draw_snake]: num_to_cord(%d, head) != 1", p->pix[i]);
    place_str(pos.x, pos.y, NULL, "\e[48;5;%dm \e[0m", p->color);
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

int snake_control(player *p){
  char ch;

  while((ch = player_getc(p)) != 'q' ){
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
        default:
          player_lock(p);
          player_unlock(p);
    }
  }
  clear_screen(p);
  destroy_player(p);
  return 0;

}

int progress_game(player *p){
  if (player_set_name(p))
    destroy_player(p);

  if ( serv_get_flags() & RANDOM_MODES &&  serv_get_flags() & ALL_MODES  ){
      serv_set_flags( serv_get_flags() & ( ~ ALL_MODES  ) );
      serv_notify_all(p->color, "SHHhhh!!! %s is joining, no more fun", p->name);
  }else{
      serv_notify_all(p->color, "%s Joined\e[00m", p->name);
  }

  clear_screen(p);
  go_home_cursor(p);
  if ( SERVER.start_banner  && ! write_file(SERVER.start_banner , p) ){
      usleep(5000000);
      clear_screen(p);
  }
  draw_board(p);
  draw_snake(p);
  serv_put_pellet(p);
  show_score(p);


  /*
   * making the player alive must come before the second thread or there is a
   * race and after any notify_alls that you don't want this player to see!!!
  */

  p->flags &= ( (int) -1 ) ^ DEAD; 

  if ( pthread_create(&p->tid_controll, &ATTR, (void *) &snake_control, p) != 0 )
      server_log(FATAL, "Could not start control thread");

  while(1){
    usleep(p->delay);
    move_snake(p);
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
  if ( head_num != serv_get_pellet() ){
    if ( snake_collision(p, head_num) ){
      game_over(p);
      destroy_player(p);
    }

    /* 
     * collisions done, remove old tail because no pellet but only if the
     * pellet does not happen to be in the tail of the snake, else it will
     * disapear 
     */
    if ( p->pix[taili] != serv_get_pellet()){
      if ( serv_get_flags() & TRASH_MODE )
        place_str( ptail.x, ptail.y, NULL,  "%c", p->name[head_num % p->nlen]);
      else
        place_str( ptail.x, ptail.y, NULL,  " ");
    }

  }else{ /* snake ate a pellet */
    grow_snake(p);
    if ( serv_get_flags() & RANDOM_MODES  && random() % 100 == 0){
      int mods = serv_random_flags(); 
      if ( mods & ( ANON_MODE | TRASH_MODE ) ){
        serv_notify_all(p->color, "MODS %s%s enabled, THANKS %s!", 
              (TRASH_MODE & mods) ?  "TRASH "  : "",
              (ANON_MODE & mods)  ?  "ANON  "  : "",
              p->name
            );
      }
    }
    serv_put_pellet(NULL);

    taili = p->head-1;
    if ( taili < 0 ) taili = taili + p->slen;
  }

  if (SERVER.flags & ANON_MODE)
    place_str(phead.x, phead.y, NULL, "\e[48;5;%dm \e[0m", 10);
  else
    place_str(phead.x, phead.y, NULL, "\e[48;5;%dm%c\e[0m", p->color, p->name[head_num % p->nlen]);

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

