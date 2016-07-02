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

void player_kill_bot(player *p){
}

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

char player_getc(player *p){
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

  if ( p->score > 0 )
      server_log(INFO, "Player \"%s\" %p %s:%d scored %d", 
      p->name, p,
      inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port), 
      p->score);

  if ( serv_check_highscore(p) )
    serv_notify_all("\e[38;5;%dm%s HIGH SCORE got %d\e[0m ", p->color, p->name, p->score);
  else
    serv_notify_all("\e[38;5;%dm%s DIES with %d\e[0m ", p->color, p->name, p->score);

  if ( p->name != NULL ) free(p->name);

  server_log(INFO, "part2 destroying player %p", p);
  pthread_mutex_destroy(&p->lock);

  serv_del_player(p);
  close(p->fd);
  free(p->pix);
  free(p);

  server_log(INFO, "snake is completely cleaned up");

  pthread_exit(0);
}

size_t player_term_string(char *s){
  size_t len;
  int i;
  char ch;
  for (i=0; i<MAX_PLAYER_NAME; i++){
    ch = s[i];
    /* server_log(INFO, "[player_term_string] checking character %02x", ch & 0xff); */
    if (ch == 0x0d || ch == 0x00) break;

    if ( ch >= (int) *"a" && ch <= (int) *"z" ) continue; /* a-z fine*/
    if ( ch >= (int) *"A" && ch <= (int) *"Z" ) continue; /* A-Z fine*/
    if ( ch >= (int) *"0" && ch <= (int)* "9" ) continue; /* 0-9 fine*/

    server_log(INFO, "[player_term_string] invalid char %02x", ch & 0xff);
    s[0] = 0x00;
    return 0;
  }
  len = i;
  s[i] = 0x00;
  return len;

}

int player_write(player *p, char *msg){
    int ret;
    ret = write(p->fd, msg, strlen(msg));
    return ret;
}

/* should not nead locks, 1 thread at this point */
int player_set_name(player *p){
  size_t len=0;
  p->name = (char *) malloc(MAX_PLAYER_NAME);
  if ( p->name == NULL ){
    server_log(ERROR, "[player_set_name] malloc() failed");
    player_write(p, "Sorry friend, got an error... try again later?\n");
    return 1;
  }
  player_write(p, "username: ");
  read(p->fd, p->name, MAX_PLAYER_NAME-1);

  len = player_term_string(p->name);

  if ( len == 0 ){
    player_write(p, "Invalide username");
    server_log(INFO, "Invalid username");
    return 1;
  }
  if ((p->name = realloc(p->name, len)) == NULL){
    server_log(ERROR, "[player_set_name] realloc() failed");
    player_write(p,   "Sorry friend, got an error... try again later?\n");
    return 1;
  }

  /* dumb bot trying to get lucky... I am not that type of game! */
  if ( len > 4 ){
    if (len == 6 && strcmp("nobody",  p->name) == 0) goto its_a_bot;
    if (len == 5 && strcmp("admin", p->name) == 0) goto its_a_bot;
  }else{
    if (len == 4 && strcmp("root",  p->name) == 0) goto its_a_bot;
    if (len == 2 && strcmp("sa",    p->name) == 0) goto its_a_bot;
  }

  /* you got here, all is well */
  server_log(INFO, "New player %s", p->name);
  return 0;

  its_a_bot: { /* lets get ask him what his favoirt password is */
      char buff[32];
      player_write(p, "password: ");
      read(p->fd, buff, sizeof(buff));
      len = player_term_string(buff);
      if ( len == 0 ) return 2;
      player_write(p, "Go away jerk, you did not even cry at bambi\n");
      server_log(INFO, "Player %p %s:%d attempted to \"log in\" with creds (%s:%s)", 
        p, inet_ntoa(p->addr->sin_addr), ntohs(p->addr->sin_port),
        p->name, buff);
      return 1;
  }
}

player * init_player(){
  static int color = 0;
  color = (color+111) % 215;  /* should cycle well, euler and all */
  player *play = (player * ) malloc(sizeof(player));
  if ( play == NULL ) server_log(FATAL, "could not make player");

  /* start the player half dead because of one thread, easier to kill*/
  play->flags  = DEAD;   
  play->delay  =  100000;
  play->name   =  NULL;
  play->color  =  16+color;
  play->score  =  0;
  play->size   =  100;
  play->pix    =  malloc(sizeof(int)  *  play->size);

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

