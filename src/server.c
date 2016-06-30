#include  <pthread.h>
#include  <stdlib.h>
#include  "data_types.h"
#include  "logging.h"
#include  "server.h"
#include <string.h> /*strlen*/
#include <unistd.h> /*sleep*/
#include  <sys/socket.h>  /*inet_ntoa*/
#include  <netinet/in.h>  /*inet_ntoa*/
#include  <arpa/inet.h>   /*inet_ntoa*/
#include  "player.h"

void init_server(){
  /* TODO: init from config file */
  SERVER.max_y         =  0;
  SERVER.max_x         =  0;
  SERVER.port          =  0;
  SERVER.high_score    =  0;
  SERVER.t_inc         =  1000;
  SERVER.log           =  stderr;
  SERVER.start_banner  =  NULL;
  SERVER.last_player   =  -1;
  SERVER.max_players   =  5;

  if (pthread_mutex_init(&SERVER.lock, NULL) != 0)
      server_log(FATAL, "[init_server] failed mutex init");

  SERVER.players =  (player ** ) malloc(sizeof(void *) * ( SERVER.max_players+1 ) );
  if ( SERVER.players == NULL ) 
    server_log(FATAL, "[init_server] can't allocate player_array");

  /* null terminate array */
  SERVER.players[0] = NULL;

}

/* 
 * kinda silly but I expect I will be happy to have locks in one place later
*/
void serv_lock(){
  pthread_mutex_lock(&SERVER.lock);
}

void serv_unlock(){
  pthread_mutex_unlock(&SERVER.lock);
}

void debug_player_array(char *msg){
  int i;
  server_log(INFO, "%s total players: %d", msg, SERVER.last_player+1);
  for (i=0; i<=SERVER.max_players; i++)
    server_log(INFO,"\t p[%d]: %p", i, SERVER.players[i]);
}

void serv_write(char *buff){
  int i;
  int len = strlen(buff);
  serv_lock();
  for (i=0; i<=SERVER.last_player; i++)
    write(SERVER.players[i]->fd, buff, len);
  serv_unlock();
}

int serv_del_player(player *p){
  serv_lock();
  server_log(INFO, "player %p", p);
  
  if ( SERVER.last_player  == -1 ){
    server_log(INFO, "[serv_del_player] Can't del a player, no players to delet");
    serv_unlock();
    return 1;
  }

  int i;
  player *c;

  for (i=0; i<=SERVER.last_player; i++){
    c = SERVER.players[i];
    /* sanity */
    if ( c == NULL ) 
      server_log(FATAL, "[serv_del_player] found an early NULL in player array");

    /* now we got em!!!*/
    if ( c == p ){
      for (; i<=SERVER.last_player; i++){
        SERVER.players[i] = SERVER.players[i+1];
      }
    }
  }/* end loop */

  if ( i-2 != SERVER.last_player-- ){
    server_log(FATAL, "[serv_del_player] SANITY: player list corrupted");
  }
  serv_unlock();
  return 0;
}

int serv_add_player(player *p){
  serv_lock();
  if ( SERVER.max_players == SERVER.last_player ){
    server_log(INFO, "[serv_add_player] Can't add a player, too many players already");
    serv_unlock();
    return 1;
  }

  SERVER.last_player++;

  server_log(INFO, "Adding player %d", SERVER.last_player);
  SERVER.players[SERVER.last_player] = p;
  SERVER.players[SERVER.last_player+1] = NULL;
  server_log(INFO, "Adding player %d", SERVER.last_player);

  serv_unlock();
  return 0;
}

void serv_check_highscore(player *p){
  serv_lock();
  if ( ( SERVER.high_score || p->fd ) && p->score > SERVER.high_score ){
    SERVER.high_score = p->score;
    if ( p->fd ){
      server_log(INFO, "!!!NEW HIGH SCORE!!! Player %s:%d scored %d", inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port), p->score);
    }else if ( SERVER.log != stderr  ){
      server_log(INFO, "!!!NEW HIGH SCORE!!!  local Player scored %d", p->score);
    }
  }
  serv_unlock();
}

int serv_get_highscore(){
  int ret;
  serv_lock();
  ret =  SERVER.high_score;
  serv_unlock();

  return ret;
}
