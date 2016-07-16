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
#include  "menu.h"

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
   *
   * only called once for spectators and bots
  */

  /* it is a snake, and is alive. kill it and exit thread */
  if ( ( p->flags & ( DEAD | SPECTATOR ) ) == 0 ){
    server_log(DEBUG, "part1 destroying player %p:%s", p,p->name);
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

  if ( p->score > 0 ){
      server_log(INFO, "Player \"%s\" %p %s:%d scored %d", 
        p->name, p,
        inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), 
        p->score
      );
    if ( serv_check_highscore(p) )
      serv_notify_all(p->color, "NEW HIGH SCORE %s: %d", p->name, p->score);
  }else{
    if ( p->flags & SPECTATOR )
      serv_notify_all(1, "A watcher has decided to leave...");
    else
      serv_notify_all(p->color, "%s DIED", p->name, p->score);
  }

  if ( p->name == NULL ) 
    server_log(FATAL, "%s line %d p->name == NULL", __FILE__, __LINE__);
  
  pthread_mutex_destroy(&p->lock);

  /* delete player from server list */
  if ( serv_del_player(p) == -1 && serv_get_flags() & RANDOM_MODES  )
    serv_set_flags( serv_get_flags() & ( ~ ALL_MODES  ) );

  if ( p->flags & BOT )
    server_log(INFO, "FILTHY STINKING BOT %p:%s destroyed", p, p->name);
  else if ( p->flags & SPECTATOR )
    server_log(INFO, "Spectator %p:%s destroyed", p, p->name);
  else
    server_log(INFO, "Player %p:%s destroyed", p, p->name);

  free(p->name);
  close(p->fd);
  free(p->pix);
  free(p);

  pthread_exit(0);
}

/* size is tottal size of the buffer to fill at most size-1 of them */
size_t player_get_str(player *p, char *buff, size_t size, int flags){
  size_t len = 0;
  int bot_flag = 0;
  char ch;
  while (len < size-1){
    ch =  player_getc(p);  
    server_log(DEBUG, "[player_get_str] on char %d: 0x%02x", len, (ch & 0xff));
    /* 
     * valid characters add it on
    */
    if  (							
							( ch >= (int) *"a" && ch <= (int) *"z" ) ||
							( ch >= (int) *"A" && ch <= (int) *"Z" ) ||
							( ch >= (int) *"0" && ch <= (int) *"9" ) ||
							( ch == (int) *"_" )                     ||
							( ch == (int) *" " )

    ){
      server_log(DEBUG, "[player_get_str] on char %d, ACCEPTED", len);
      if ( ! ( flags & NO_FLIP_SPACE ) && ch == (int) *" " ) ch = *"_";
      buff[len]   = ch;
      if ( flags & SHADOW_CHARS )
        write(p->fd, "*", 1);
      else
        write(p->fd, &ch, 1);
      len++;
    /*
     * backspace
    */                           /* thx patrick */
		}else if ( ( ch & 0xff ) == 0x7f && len > 0){  
      len--;
      player_write(p, "\e[1D \e[1D");
    /*
     * ctrl+u
    */
		}else if ( ( ch & 0xff ) == 0x15 ){  
      len = 0;
      player_write(p, "\e[2K\e[20Dusername: ");
    /* 
     * IAC b/c they are using telnet 
    */
		}else if ( ( ch & 0xff ) == 0xff ){  
      bot_flag++;
      ch = player_getc(p);
      /* 
       * IAC: not ( WILL | WON'T | DO | DON'T  ) kick em ?
      */ 
      if ( ( ch & 0xff ) < 0xfa  || ( ch & 0xff ) > 0xfe ) { 
        server_log(DEBUG, "[player_set_name] Player %p Got IAC 0x%02x, kicking", p, ch & 0xff); 
        len = 0;
        break;
      /* subnegociation (SB), go till subnegociation ends (se) */
      }else if ( (ch & 0xff) == 0xfa ){ 
        while ( 1 ){
          ch = player_getc(p);
          if ( (ch & 0xff ) != 0xf0 )
            server_log(DEBUG, "[player_set_name] in negociation, got 0x%02x", ch & 0xff);
          else
            break;
        }
      /* IAC and valid? */
      }else{
        server_log(DEBUG, "[player_set_name] %p sent IAC 0x%02x 0x%02x", p, ch & 0xff, player_getc(p) & 0xff);
      }

    } else if ( (ch & 0xff) == 0x0d  || (ch & 0xff) == 0x00 ){   /* Terminating character, finish it up */
      ch = player_getc(p);
      server_log(DEBUG, "ending str, next char is: 0x%02x", ch & 0xff);
      break;
    } else if ( (ch & 0xff) == 0x0a ){   /* Terminating character, finish it up */
      server_log(DEBUG, "ending str, next char is: 0x%02x", ch & 0xff);
      break;
    /* 
     * bad char, kill 
    */
		}
  }
  /* sanity */
  if ( len > size ) {
    hexdump(buff, size);
    server_log(FATAL, "%s [player_set_name] line %d Apparent size_t underflow?"
                      "\n\t  len:      %zu"
                      __FILE__, __LINE__, len
    );
  }
    
  buff[len] = 0x00;

  int k = strlen(buff);
  if ( k > size-1 ){
    hexdump(buff, size);
    server_log(FATAL, "%s [player_set_name] line %d BUFFER OVERFLOW DETECTED"
                      "\n\t  size:  %d"
                      "\n\t  strlen(buff)   %d",
                      __FILE__, __LINE__, size, strlen(buff)
    );
  }
  if ( k != len ){
    hexdump(buff, size);
    server_log(FATAL, "%s [player_set_name] line %d Did not get the write name length!!!"
                      "\n\t  buff:  %s"
                      "\n\t  k:        %d"
                      "\n\t  len       %d",
                      __FILE__, __LINE__, buff, k, len
    );
  }
  /* you are sane */

  if ( ( flags & MIN_IAC_REQ ) &&  bot_flag < 2 && ( p->flags & BOT ) == 0 )
    player_is_a_bot(p);

  if ( p->name != NULL )
    server_log(DEBUG, "Player (%s) %s:%d sent %d telnet IAC bytes", p->name,
      inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), bot_flag);

  return len;
}

int player_write(player *p, char *msg){
    int ret;
    ret = write(p->fd, msg, strlen(msg));
    if ( ret == -1 )
      destroy_player(p);
    return ret;
}

int check_spectator(player *p){
  char pass[MAX_PLAYER_NAME+1];

  /* password required */
  if ( SERVER.spec_pass == NULL ){
    player_write(p, "\e[B\e[20Dpassword: ");

    if ( player_get_str(p, pass, sizeof(pass), SHADOW_CHARS) == 0 )
      return 2;

    /* hashing? telnet is plain text AND it is a snake game, who cares... */
    if ( strcmp(SERVER.spec_pass, pass))
      return 3;
  }

  p->flags |= SPECTATOR;
  serv_notify_all(10, "Someone is watching you!");
  return 0;
}


void player_is_a_bot(player *p){
  /* its a bot, lets get ask him what his favoirt password is */
  char pass[MAX_PLAYER_NAME+1];
  size_t len;
  p->flags |= BOT;

  if ( strlen(p->name) == 0 )
    destroy_player(p);

  player_write(p, "\e[B\e[20Dpassword: ");
  len = player_get_str(p, pass, sizeof(pass), SHADOW_CHARS);


  if ( len == 0 ){
    serv_notify_all(88, "Silly bot: %s:%d (%s) you need a password to login ", 
      inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), p->name);

    server_log(INFO, "Silly bot: %s:%d (%s) you need a password to login", 
      inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), p->name);
    destroy_player(p);
  }

  serv_notify_all(88, "Silly bot: %s:%d tried to login as (%s:%s)", 
    inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), p->name, pass);

  server_log(INFO, "Silly bot: %s:%d tried to login as (%s:%s)", 
    inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port), p->name, pass);

  if ( SERVER.bot_warn  && ! write_file(SERVER.bot_warn , p) )


  destroy_player(p);
}

/* should not nead locks, 1 thread at this point */
int player_set_name(player *p){
  size_t len;

  /* 
   * tell tellnet not to write characters to the screen, send every keypress,
   * and don't be such a jerk... 
  */
  player_write(p,
    "\xff\xfd\x22" /* IAC DO LINEMODE*/
    "\xff\xfb\x01" /* IAC WILL ECHO */
  ); 

  p->name = (char *) malloc(MAX_PLAYER_NAME+1);
  if ( p->name == NULL ){
    server_log(ERROR, "[player_set_name] malloc() failed");
    player_write(p, "Sorry friend, got an error... try again later?\n");
    return 1;
  }

  player_write(p, "username: ");
  len = player_get_str(p, p->name, MAX_PLAYER_NAME+1, MIN_IAC_REQ);

  if ( len == 0 ){
    player_write(p, "Invalid username");
    return 1;
  }

  if ((p->name = realloc(p->name, len+1)) == NULL){
    server_log(FATAL, "%s [player_set_name] line %d realloc",__FILE__, __LINE__);
    return 1;
  }
  if ( SERVER.spec_name && strcmp(p->name, SERVER.spec_name) == 0 )
    return check_spectator(p);

  /* dumb bot trying to get lucky... I am not that type of game! */
  if ( len > 4 ){
    if (len == 6 && strcmp("888888",  p->name) == 0)   player_is_a_bot(p);
    if (len == 6 && strcmp("nobody",  p->name) == 0)   player_is_a_bot(p);
    if (len == 5 && strcmp("admin", p->name) == 0)     player_is_a_bot(p);
    if (len == 5 && strcmp("guest", p->name) == 0)     player_is_a_bot(p);
    if (len == 7 && strcmp("support", p->name) == 0)   player_is_a_bot(p);
  }else{
    if (len == 4 && strcmp("ubnt",  p->name) == 0)     player_is_a_bot(p);
    if (len == 4 && strcmp("root",  p->name) == 0)     player_is_a_bot(p);
    if (len == 2 && strcmp("sa",    p->name) == 0)     player_is_a_bot(p);
  }

  /* you got here, all is well */
  p->nlen = len;
  server_log(INFO, "New Player %s (%p) %s:%d ", p->name, 
    p, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port)
  );
  return 0;

}
player * init_player(){
  static int color = 0;
  color = (color+111) % 215;  /* should cycle well, euler and all */
  player *play = (player * ) malloc(sizeof(player));
  if ( play == NULL ) server_log(FATAL, "could not make player");

  /* 
   * start the player half dead, makes it easier to kill and the server won't
   * talk to you when it talks to everyone
  */
  play->flags  =  DEAD;   
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

