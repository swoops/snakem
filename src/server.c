#include  <pthread.h>
#include  <stdlib.h>
#include  "data_types.h"
#include  "logging.h"
#include  "server.h"
#include  <stdarg.h>      /*va_start*/
#include  <string.h>      /*strlen*/
#include  <unistd.h>      /*sleep*/
#include  <sys/socket.h>  /*inet_ntoa*/
#include  <netinet/in.h>  /*inet_ntoa*/
#include  <arpa/inet.h>   /*inet_ntoa*/
#include  "player.h"
#include  "snake.h"
#include  "menu.h"
#include  "movement.h"

void init_server(){
  /* TODO: init from config file */
  SERVER.mods          =  0;
  SERVER.rand_mods     =  0;
  SERVER.max_y         =  0;
  SERVER.max_x         =  0;
  SERVER.uid           =  0;
  SERVER.port          =  4444;
  SERVER.high_score    =  0;
  SERVER.t_inc         =  1000;
  SERVER.log           =  stderr;
  SERVER.start_banner  =  NULL;
  SERVER.bot_warn      =  NULL;
  SERVER.last_player   =  -1;
  SERVER.max_players   =  5;

  SERVER.num_bnames    =  0;
  SERVER.bnames        =  NULL;


  SERVER.spec_name = NULL;
  SERVER.spec_pass = NULL;

  SERVER.hs_name   = strdup("Nobody");
  if (SERVER.hs_name == NULL)
    server_log(FATAL, "%s [init_server]  line:%d ", __FILE__, __LINE__ );

  if (pthread_mutex_init(&SERVER.lock, NULL) != 0)
      server_log(FATAL, "[init_server] failed mutex init");

  SERVER.players =  (player ** ) malloc(sizeof(void *) * ( SERVER.max_players+1 ) );
  if ( SERVER.players == NULL ) 
    server_log(FATAL, "[init_server] can't allocate player_array");

  /* null terminate array */
  SERVER.players[0] = NULL;

}
void serv_destory(){
  /* free up all illegal names and the older for them */
  if ( SERVER.num_bnames > 0 ){
    int i;
    for (i=0; i<SERVER.num_bnames; i++){
      free( SERVER.bnames[i] );
    }
    free(SERVER.bnames);
  } 
  if ( SERVER.start_banner !=  NULL  ) free(SERVER.start_banner);
  if ( SERVER.bot_warn     !=  NULL  ) free(SERVER.bot_warn);
  if ( SERVER.spec_name    !=  NULL  ) free(SERVER.spec_name);
  if ( SERVER.spec_pass    !=  NULL  ) free(SERVER.spec_pass);
  if ( SERVER.hs_name      !=  NULL  ) free(SERVER.hs_name);
  if ( SERVER.players      !=  NULL  ) free(SERVER.players);
  if ( SERVER.log          != stderr ) fclose(SERVER.log);

  pthread_mutex_destroy(&SERVER.lock);
}

void server_kill_em_all(int wait){
  int i;
  serv_notify_all(0, "The server is trying to kick all players, run!!!");
  serv_lock();
  for (i=0; i<=SERVER.last_player; i++){
    pthread_mutex_lock(&SERVER.players[i]->lock);
    SERVER.players[i]->flags |= KILL; 

    /* asks the player to say something, forcing a lock and thus a death check */
    player_ayt(SERVER.players[i]);
    pthread_mutex_unlock(&SERVER.players[i]->lock);
  }
  serv_unlock();

  if ( wait ) {
    serv_wait_on_players();
    /* give time for the last 2 free()'s in destroy_player */
    sleep(1); 
  }
}

/* this waits on all the player threads to die */
/* locking via serv_get_num_players */
void serv_wait_on_players(){
  server_log(DEBUG, "Waiting on players: %d", SERVER.last_player);
  while ( serv_get_num_players() > 0){
      server_log(DEBUG, "sleeping 1 second");
      sleep(1);
  }
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

#ifdef PLAYER_ARRAY_DBUG
void debug_player_array(char *msg){
  int i;
  server_log(INFO, "%s total players: %d", msg, SERVER.last_player+1);
  for (i=0; i<=SERVER.max_players; i++)
    server_log(INFO,"\t p[%d]: %p", i, SERVER.players[i]);
}
#endif

int serv_check_collisions(player *p, int head){
  int i;
  serv_lock();
  for (i=0; i<=SERVER.last_player; i++){
    pthread_mutex_lock(&SERVER.players[i]->lock);
    if (SERVER.players[i]->flags | DEAD ){
      if ( snake_collision(SERVER.players[i], head) ){
        /* collision so clean and return */
        serv_unlock();
        SERVER.players[i]->score += 100;
        show_score(SERVER.players[i]);
        pthread_mutex_unlock(&SERVER.players[i]->lock);

        if ( SERVER.players[i] == p )
          serv_notify_all(SERVER.players[i]->color, "%s killed himself", p->name );
        else
          serv_notify_all(SERVER.players[i]->color, "%s killed %s", SERVER.players[i]->name, p->name );
        return 1;
      }
    }
    pthread_mutex_unlock(&SERVER.players[i]->lock);
  }
  serv_unlock();
  return 0;
}

/* locks server and all players */
void serv_write(char *buff){
  int i;
  int len = strlen(buff);
  serv_lock();
  /* more effecent to use write() so strlen() runs once */
  for (i=0; i<=SERVER.last_player; i++){
    pthread_mutex_lock(&SERVER.players[i]->lock);
    if ( ( SERVER.players[i]->flags & DEAD ) == 0 )
      write(SERVER.players[i]->fd, buff, len);
    pthread_mutex_unlock(&SERVER.players[i]->lock);
  }
  serv_unlock();
}

int serv_del_player(player *p){
  serv_lock();
  
  if ( SERVER.last_player  == -1 ){
    server_log(INFO, "[serv_del_player] Can't del a player, no players to delet");
    serv_unlock();
    return -2;
  }

  int i, ret;
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

  server_log(INFO, "removed player num %d:%p from %s:%d", 
    SERVER.last_player+1, p,
    inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port)
  );

  ret = SERVER.last_player;
  serv_unlock();
  return ret;
}

int serv_add_player(player *p){
  serv_lock();
  if ( SERVER.max_players == SERVER.last_player ){
    server_log(INFO, "[serv_add_player] Can't add a player, too many players already");
    serv_unlock();
    return 1;
  }

  SERVER.last_player++;

  server_log(INFO, "Adding player num %d:%p from %s:%d", 
    SERVER.last_player, p,
    inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port)
  );
  SERVER.players[SERVER.last_player] = p;
  SERVER.players[SERVER.last_player+1] = NULL;

  serv_unlock();
  return 0;
}

int serv_check_highscore(player *p){
  int flag = 0;
  serv_lock();
  if ( p->score > SERVER.high_score ){
    flag = 1;
    SERVER.high_score = p->score;
    server_log(INFO, "!!!NEW HIGH SCORE!!! Player %s %s:%d scored %d", 
      p->name, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), p->score);

    /* update SERVER with name of current high score holder */
    if ( SERVER.hs_name != NULL )
      free(SERVER.hs_name);
    SERVER.hs_name = strdup(p->name);
  }

  serv_unlock();
  return flag;
}
int serv_full(){
  int ret = 0;
  serv_lock();
  if ( SERVER.max_players == SERVER.last_player+1 )
    ret++;
  serv_unlock();
  return ret;
}

/*
 * make sure pellet not placed in a snake
*/
void serv_put_pellet(player *p){
  point pellet;

  serv_lock();

  if (p == NULL)
    SERVER.pellet = random() % ( ( SERVER.max_y-4 ) * (SERVER.max_x-5 ) );

  if ( num_to_cord(SERVER.pellet, &pellet)  == 0 )
    server_log(ERROR, "[serv_put_pellet]: pellet out of bounds (%d, %d): %d", pellet.x, pellet.y, SERVER.pellet);

  serv_unlock();
  
  place_str( pellet.x, pellet.y, p, "\e[38;5;0;48;5;46m*\e[0m");
}

void serv_random_mods(char *name, int chance){
  if ( name && ( random() % chance ) != 0 ) return;

  server_log(DEBUG, "serv_random_mods( %s, %d )",
    (name) ? name : "NULL",
    chance
  );

  serv_lock();
  int i = SERVER.rand_mods;

  /* is randome enabled? */

  if ( ( SERVER.mods & RANDOM_MODES ) == 0 ){
    serv_unlock();
    return;
  }
  SERVER.rand_mods =  ( ( random() & ALL_MODES )  | SERVER.mods ) ;
  serv_unlock();

  server_log(INFO, "[serv_random_mods] flags set %s%s%s%s by %s", 
    (S_INVISIBO & i)    ?  "INVISIBO "      : "",
    (TRASH_MODE & i)    ?  "TRASH_MODE "    : "",
    (ANON_MODE & i)     ?  "ANON_MODE "     : "",
    (RANDOM_MODES & i)  ?  "RANDOM_MODES "  : "",
    (name)              ? name : "watcher"
  );

  /* must be unlocked before calling notify or deadlock will ensue */
  if ( SERVER.rand_mods != i )
    serv_notify_all(0, "MODS %s%s%s enabled, THANKS %s!", 
      (S_INVISIBO & i) ?  "INVISIBO "      : "",
      (TRASH_MODE & i) ?  "TRASH "  : "",
      (ANON_MODE & i)  ?  "ANON  "  : "",
      (name)           ?  name      : "watcher"
    );
}

void serv_clear_mods(){
  serv_lock();
  SERVER.rand_mods = SERVER.mods;
  serv_unlock();
}

/* locks server and all players */
void serv_notify_all(int color, char *fmt, ...) {
  /* 
   * BE CAREFULL CHANGING THIS FUNCTION !!!
   *           it is stupid
  */

  #define PT_1_STR "\e[%dH\e[L \e[38;5;%dm" /* go to position, new line, color */
  #define TMP_CLOSE_STR "\e[00m\e[4B\e[2K\e[H"  /* end color, go down, clear line, go home*/
  int i;
  int max_str = sizeof(PT_1_STR) + 6          /* len PT_1_STR + len %d *2 */
              + SERVER.max_x + 64             /* width of screen, plus some room for encoding ie: vsnprintf */
              + sizeof(TMP_CLOSE_STR)         /* end encoding */
              + 1;                            /* null of course */
  char buff[max_str];
  va_list ap;

  if ( color < 17 || color > 231 )
    color = 10;  /* GREEN like a term should be!!!! */

  i = sprintf(buff, PT_1_STR, SERVER.max_y+1, color);
  #undef PT_1_STR

  va_start(ap, fmt);
  /* leave room for the closing string */
  vsnprintf(buff+i, max_str-i-sizeof(TMP_CLOSE_STR)-1, fmt, ap);
  va_end(ap);

  i = strlen(buff);

  /* sanity, should not be == b/c need room for NULL */
  if ( i+sizeof(TMP_CLOSE_STR) >= max_str  )
    server_log(FATAL, 
      "%s [serv_notify_all]  line: %d BUFFER OVERFLOW DETECTED!!!"
      "\n\ti:                     %d"
      "\n\tsizeof(TMp_CLOSE_STR): %d"
      "\n\tmax_str:               %d"
      , __FILE__, __LINE__,i, sizeof(TMP_CLOSE_STR), max_str 
    );
  snprintf(buff+i, max_str-i, TMP_CLOSE_STR);
  serv_write(buff);
  #undef TMP_CLOSE_STR
}

/* server GETs */
/* TODO: abstractt his to one function */
int serv_get_highscore(){
  int ret;
  serv_lock();
  ret =  SERVER.high_score;
  serv_unlock();

  return ret;
}
int serv_get_mods(){
  int ret;
  serv_lock();
  ret = SERVER.rand_mods;
  serv_unlock();
  return ret;
}
int serv_get_num_players(){
  int ret;
  serv_lock();
  ret = SERVER.last_player +1;
  serv_unlock();
  return ret;
}

int serv_get_pellet(){
  int pellet;
  serv_lock();
  pellet = SERVER.pellet;
  serv_unlock();
  return pellet;
}

