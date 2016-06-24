#include <pthread.h>
#include <string.h> /*strlen*/
#include <stdlib.h> /*malloc*/
#include <sys/socket.h> /*inet_ntoa*/
#include <netinet/in.h> /*inet_ntoa*/
#include <arpa/inet.h> /*inet_ntoa*/
#include  "data_types.h"
#include  "player.h"
#include "logging.h"

void player_unlock(player *p){
  /* should not unlock a player that is NULL? */
  if (p->flags & DEAD) server_log(ERROR, "unlocking a dead snake");

  pthread_mutex_unlock(&p->lock);

}

void player_write(player *p, char *buff){
  if (p->fd != 0){
    write(p->fd, buff, strlen(buff));
  }else{
    write(1, buff, strlen(buff));
  }
}

char pgetc(player *p){
    char ch;
    read(p->fd, &ch, 1);
    return ch;
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


  /* snake is alive, kill it and exit thread */
  if ( ( p->flags & DEAD ) == 0 ){
    pthread_mutex_lock(&p->lock);
    p->flags |= DEAD;
    pthread_mutex_unlock(&p->lock);

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
    server_log(INFO, "Player %s:%d scored %d", inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port), p->score);
    close(p->fd);
  }else if ( SERVER.log != stderr  ){
    server_log(INFO, "local Player scored %d", p->score);
  }

  /* high score check */
  if ( ( SERVER.high_score || p->fd ) && p->score > SERVER.high_score ){
    SERVER.high_score = p->score;
    if ( p->fd ){
      server_log(INFO, "!!!NEW HIGH SCORE!!! Player %s:%d scored %d", inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port), p->score);
      close(p->fd);
    }else if ( SERVER.log != stderr  ){
      server_log(INFO, "!!!NEW HIGH SCORE!!!  local Player scored %d", p->score);
    }
  }

  free(p->pix);
  free(p);

  if ( SERVER.log != stderr  )
    server_log(INFO, "snake is completely cleaned up");

  pthread_exit(0);
}


player * init_player(){
  player *play = (player * ) malloc(sizeof(player));
  if ( play == NULL ) server_log(FATAL, "could not make player");

  play->color = 0;
  play->score = 0;
  play->size = 100;
  play->flags = 0;
  play->fd = 0;  /* assume stdin */
  play->pix  =  malloc(sizeof(int) * play->size);
  if ( play->pix == NULL ){
    free(play);
    server_log(FATAL, "could not get space for snake pix");
  }
  play->slen = 3;
  int i;
  for (i=play->slen; i>0; i--)
    play->pix[play->slen-i] = i;
  play->dir = RIGHT;
  play->head = 0;

  return play;
}

