#include<pthread.h>
#include  "data_types.h"
#include  "menu.h"

void player_unlock(player *p){
  /* should not unlock a player that is NULL? */
  if (p->pix == NULL) fatal("unlocking null player")

  pthread_mutex_unlock(&p->lock);

}
void player_lock(player *p){
  if (p->pix == NULL) /* someone killed the player already */
    pthread_exit(0);

  pthread_mutex_lock(&p->lock);
}

void destory_player(player *p){
  /*
   * cleans up the player struct...  nicely?
   * save current player pix pointer locally and set the one everyone knows to
   * NULL, lock/unlock wrappers should catch the NULL reference and this should
   * stop race conditions
  */


  player * op = p;
  *p = NULL; 

  pthread_mutex_destroy(&op->lock);

  if ( play->fd ){
    close(play->fd);
    if (SERVER.log){
      server_log("Player %s:%d scored %d\n", inet_ntoa(p->addr->sin_addr), ntohs(op->addr->sin_port), op->score);
    }
  }

  free(p->pix);
  free(p);
  p = NULL;
  return 0;
}

player * init_player(){
  player *play = (player * ) malloc(sizeof(player));
  if ( play == NULL ) fatal("could not make player\n");

  play->color = 0;
  play->score = 0;
  play->size = 100;
  play->fd = 0;  /* assume stdin */
  play->pix  =  malloc(sizeof(int) * play->size);
  if ( play->pix == NULL ){
    free(play);
    fatal("could not get space for snake pix\n");
  }
  play->slen = 3;
  int i;
  for (i=play->slen; i>0; i--)
    play->pix[play->slen-i] = i;
  play->dir = RIGHT;
  play->head = 0;

  return play;
}

