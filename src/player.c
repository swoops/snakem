#include  <pthread.h>
#include  <string.h>      /*strlen*/
#include  <stdlib.h>      /*malloc*/
#include  <sys/socket.h>  /*inet_ntoa*/
#include  <netinet/in.h>  /*inet_ntoa*/
#include  <arpa/inet.h>   /*inet_ntoa*/
#include  "data_types.h"
#include  "server.h"
#include  "player.h"
#include  "logging.h"

void player_unlock(player *p){
  /* should not unlock a player that is NULL? */
  if (p->flags & DEAD) server_log(ERROR, "unlocking a dead snake");

  pthread_mutex_unlock(&p->lock);

}

void player_lock(player *p){
  pthread_mutex_lock(&p->lock);
  if (p->flags & ( DEAD | KILL )){/* IT'S DEAD!.. WILL!!! || kill it */
    pthread_mutex_unlock(&p->lock);
    destroy_player(p);
  } 

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

void destroy_player(player *p){
  /*
   * called twice, per snake to exit, once per each thread first call marks
   * snake as dead, second call cleans up the mess
  */

  /* snake is alive, kill it and exit thread */
  if ( ( p->flags & DEAD ) == 0 ){
    server_log(INFO, "part1 destroying player %p", p);
    pthread_mutex_lock(&p->lock);
    p->flags |= DEAD;

    /* get a response to kill controle thread if it is still there*/
    player_write(p, 
      "\xff\xfb\xf6" /* IAC WILL AYT*/
    ); 

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
  server_log(INFO, "Player %p:%s:%d scored %d", p, inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port), p->score);

  server_log(INFO, "part2 destroying player %p", p);
  pthread_mutex_destroy(&p->lock);

  serv_check_highscore(p);
  serv_del_player(p);
  close(p->fd);
  free(p->pix);
  free(p);

  server_log(INFO, "snake is completely cleaned up");

  pthread_exit(0);
}


player * init_player(){
  static int color = 0;
  color = (color+111) % 215;  /* should cycle well, euler and all */
  player *play = (player * ) malloc(sizeof(player));
  if ( play == NULL ) server_log(FATAL, "could not make player");

  play->color = 16+color;
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

