#include <pthread.h>
#include <stdlib.h> /*malloc*/
#include <sys/socket.h> /*inet_ntoa*/
#include <netinet/in.h> /*inet_ntoa*/
#include <arpa/inet.h> /*inet_ntoa*/
#include  "data_types.h"
#include  "player.h"
#include "logging.h"

void player_unlock(player *p){
  /* should not unlock a player that is NULL? */
  if (p->flags & DEAD) fatal("unlocking a dead snake");

  pthread_mutex_unlock(&p->lock);

}
void player_lock(player *p){
  if (p->flags & DEAD) /* IT'S DEAD!.. WILL!!! */
    destroy_player(p);

  pthread_mutex_lock(&p->lock);
}

void destroy_player(player *p){
  /*
   * called twice, per snake to exit, once per each thread first call marks
   * snake as dead, second call cleans up the mess
  */

  if (SERVER.log) server_log("In destroy_player: snake is dead?: %d\n", p->flags);
  if (SERVER.log) server_log("p->flags & DEAD == %d\n", (p->flags & DEAD));

  /* snake is alive, kill it and exit thread */
  if ( ( p->flags & DEAD ) == 0 ){
    if (SERVER.log) server_log("in if statment: snake is dead?: %d\n", p->flags);
    pthread_mutex_lock(&p->lock);
    p->flags |= DEAD;
    pthread_mutex_unlock(&p->lock);
    if (SERVER.log) server_log("first thread exiting\n");
    pthread_exit(0);
    return;
  }

  /* snake is dead...
   * you are the only one who remembers him
   * clean him up and exit
   * it is what he would of wanted.
   * at least he won't be in our memory
  */

  pthread_mutex_destroy(&p->lock);

  if ( p->fd ){
    close(p->fd);
    if (SERVER.log) server_log("Player %s:%d scored %d\n", inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port), p->score);
  }

  free(p->pix);
  free(p);

  if (SERVER.log) server_log("snake is completely cleaned up\n");
  pthread_exit(0);
}

player * init_player(){
  player *play = (player * ) malloc(sizeof(player));
  if ( play == NULL ) fatal("could not make player\n");

  play->color = 0;
  play->score = 0;
  play->size = 100;
  play->flags = 0;
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

