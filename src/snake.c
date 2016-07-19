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

char *MESSAGES[] = {
  "No, houseboat, you forgot to add the C",
  "I'm the best!!!",
  "Mirupafshim",
  "Swoops",
  "Do a flip!",
  "check this out n00bs!",
  "Ctrl+] to talk",
  "Take that causality!",
  "What do you call the integral of 1/cabin?",
  "log cabin?"
};


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
        case '0': /* someone wants to send a messege */
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          serv_notify_all(p->color, "%s: %s", p->name, MESSAGES[atoi(&ch)]);
          break;
        case 0x00:
          destroy_player(p);
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

/* TODO:
 * currently this accepts a player struct which is fine for now because
 * player_init starts the player with min slen, this keeps the server from
 * attempting to find collisions... however it would be better to make a
 * spectator struct that does not contain as much needless junk, pixels, slen,
 * head, ect. Don't put that spectator inside of the list of players, destroy
 * him differently, and make sure server_write loops through the spectators for
 * write also.  For now this hack *works* but by accident. 
 */ 
int snake_spectate(player *p){
  char ch;
  int mods;
  char msgbuff[140];

  clear_screen(p);
  draw_board(p);
  serv_put_pellet(p);

  /* no longer dead, so it gets serv_notify_all msgs */
  p->flags &= ( (int) -1 ) ^ DEAD; 

  while((ch = player_getc(p)) != 'q' ){
    switch(ch){
        case 'c':
          clear_screen(p);
          draw_board(p);
          serv_put_pellet(p);
          break;
        /* send a message to everyone */
        case 'm':
          p->flags |= DEAD; /* don't talk while I am typing */
          player_write(p, "\e[2J\e[HMSG: ");
          if ( player_get_str(p, msgbuff, sizeof(msgbuff), NO_FLIP_SPACE) != 0 )
            serv_notify_all(10, "watcher: %s", msgbuff);
          draw_board(p);
          serv_put_pellet(p);
          p->flags &= ( (int) -1 ) ^ DEAD; 
          break;
        case 'r':
          if ( serv_get_flags() & RANDOM_MODES ){
            mods = serv_random_flags(); 
            if ( mods & ( ANON_MODE | TRASH_MODE ) ){
              serv_notify_all(88, "MODS %s%s enabled by the watcher",
                    (TRASH_MODE & mods) ?  "TRASH "  : "",
                    (ANON_MODE & mods)  ?  "ANON  "  : ""
              );
            }
          }
          break;
        case 0x00:
          server_log(DEBUG, "spectator is gone");
          destroy_player(p);
          break;
        default:
          sleep(1);
          server_log(DEBUG, "Spectator pressed: 0x%02x", ch&0xff);
          break;
    }
  }
  clear_screen(p);
  destroy_player(p);
  return 0;
}

int progress_game(player *p){
  if (player_set_name(p))
    destroy_player(p);

  /* player just wants to watch */
  if ( p->flags & SPECTATOR )
    snake_spectate(p);

  if ( serv_get_flags() & RANDOM_MODES &&  serv_get_flags() & ALL_MODES  ){
      serv_set_flags( serv_get_flags() & ( ~ ALL_MODES  ) );
      serv_notify_all(p->color, "SHHhhh!!! %s is joining, no more fun", p->name);
  }else{
      serv_notify_all(p->color, "%s Joined\e[00m", p->name);
  }

  clear_screen(p);
  go_home_cursor(p);
  if ( SERVER.start_banner  && ! write_file(SERVER.start_banner , p) ){
      if ( player_getc(p) == 0x00) destroy_player(p);
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
  player_lock(p);
  if ( p->size == p->slen ){
    winner(p);  // TODO: realloc memory so no one wins... ever...
    destroy_player(p);
  }
  int i;
  for ( i=p->slen++; i >= p->head; i-- )
    p->pix[i+1] = p->pix[i];
  p->head = ( p->head+1 ) % p->slen;
  player_unlock(p);

  p->delay -= SERVER.t_inc;  // go faster

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
    if ( p->slen > 3 && serv_check_collisions(p, head_num) ){  /* players <3 invincible :) */
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
  player_lock(p);
  p->head = taili;
  p->pix[p->head] =  head_num;
  player_unlock(p);

}
/* TODO: make debug a compile flag*/

int snake_collision(player *p, int col){
  /* loop might seem strange, this diagram should help understand the conversion
   * |original|   |what I loop|   |i|     
   * pix[0] *     pix[5]  h  <-  ( 0 + p->head  ) % slen <- once i < 0 you are done
   * pix[1] *     pix[6]  *  <-  ( 1 + p->head  ) % slen
   * pix[2] *     pix[7]  *  <-  ( 2 + p->head  ) % slen
   * pix[3] *     pix[8]  *  <-  ( 3 + p->head  ) % slen
   * pix[4] t     pix[9]  *  <-  ( 4 + p->head  ) % slen
   * pix[5] h  >  pix[0]  *  <-  ( 5 + p->head  ) % slen
   * pix[6] *     pix[1]  *  <-  ( 6 + p->head  ) % slen
   * pix[7] *     pix[2]  *  <-  ( 7 + p->head  ) % slen
   * pix[8] *     pix[3]  *  <-  ( 8 + p->head  ) % slen
   * pix[9] *     pix[4]  t  <-  ( 9 + p->head  ) % slen <- start on tail
  */
  /*
   * handle when col is the head of your own snake
  */
  int i,dist;
  point pos1, pos2;

  for (i=p->slen-1; i>=0; ){
    if ( num_to_cord(col, &pos1) != 1)
      server_log(ERROR, "%s:%d Point outside of bounsd", __FILE__, __LINE__);
    if ( num_to_cord(p->pix[(i+p->head)%p->slen], &pos2) != 1)
      server_log(ERROR, "%s:%d Point outside of bounsd", __FILE__, __LINE__);

    /* Manhattan/Taxi Cab distance */
    dist = abs(pos1.x-pos2.x)+ abs(pos1.y-pos2.y);

    if ( dist == 0 ) /* collision */
      return 1;

    /* 
     * if the i'th position is, lets, say 3, away from the collision point.
     * That means you have to move at LEAST 3 pixels to collide.  So the next
     * two pixels in the snake can not collide, skip over them :)
    */

    i -= dist;

  }

  return 0;
}
