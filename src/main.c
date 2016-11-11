#include  <stdio.h>
#include  <stdlib.h>
#include  <pthread.h>
#include  "snake.h"
#include  <string.h>        /*memset*/
#include  <arpa/inet.h>     /*htons*/
#include  <signal.h>        /*sigemptyset,sigaction*/
#include  <sys/types.h>     /*umask*/
#include  <sys/stat.h>      /*umask*/
#include  <errno.h>         /*errno*/
#include  <sys/socket.h>
#include  "player.h"
#include  "logging.h"
#include  "server.h"
#include  "parse_config.h"
#include  "data_types.h"

/* to decide what to do after a signal */
#define  SIG_CODE_KILL_ALL  1
#define  SIG_CODE_SLOW      2
#define  SIG_CODE_DISPLAY   3
static int MY_SIG_CODE;

static void caught_sig(int sig){
  server_log(INFO, "[caught_sig] caught signal %d", sig);
  if ( MY_SIG_CODE != 0 ) exit(1);

  if ( sig == SIGTERM   )
    MY_SIG_CODE = SIG_CODE_KILL_ALL;
  else if ( sig == SIGUSR1 )
    MY_SIG_CODE = SIG_CODE_SLOW;
  else if ( sig == SIGUSR2 || sig == SIGINT)
    MY_SIG_CODE = SIG_CODE_DISPLAY;
}

static void set_signal(){
  /*
   * parent signal regisers a handler for:
   *  - SIGUSR1 stop any new connection and clean
   *  - SIGTERM kick players as if they died, and clean
   *  
   * clean as in close files, free memmory, record high scores, log events, 
   * and close nicely
  */
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGPIPE, &sa, 0) == -1) 
    server_log(FATAL, "Could not change SIGPIPE singal");

  sa.sa_handler = caught_sig;
  if (sigaction(SIGTERM, &sa, 0) == -1) 
    server_log(FATAL, "Could not change SIGTERM singal");
  if (sigaction(SIGINT, &sa, 0) == -1) 
    server_log(FATAL, "Could not change SIGINT singal");
  if (sigaction(SIGUSR1, &sa, 0) == -1) 
    server_log(FATAL, "Could not change SIGUSR1 singal");
  if (sigaction(SIGUSR2, &sa, 0) == -1) 
    server_log(FATAL, "Could not change SIGUSR2 singal");
}

static void check_privs(){
  uid_t rid, eid;

  rid = getuid();
  if ( getgid() == 0 )
    rid = 0;

  eid = geteuid();
  if ( getegid() == 0 )
    eid = 0;


  if (rid != 0 && eid != 0)
    return;

  if ( rid == 0 && eid == 0 )
    server_log(FATAL, "[check_privs] %s:%d: bad privs", __FILE__, __LINE__);

  /* whoever is not root, be them */
  if ( rid == 0 ){
    if (setgid(eid) != 0)
      server_log(FATAL, "[check_privs] %s:%d: failed to drop group privs", __FILE__, __LINE__);
    if (setuid(eid) != 0)
      server_log(FATAL, "[check_privs] %s:%d: failed to drop user privs", __FILE__, __LINE__);
  }else{
    if (setgid(rid) != 0)
      server_log(FATAL, "[check_privs] %s:%d: failed to drop group privs", __FILE__, __LINE__);
    if (setuid(rid) != 0)
      server_log(FATAL, "[check_privs] %s:%d: failed to drop user privs", __FILE__, __LINE__);
  }

  umask(0);

  if (setuid(0) != -1)
    server_log(FATAL, "[check_privs] %s:%d: got root somehow...", __FILE__, __LINE__);
}

static void make_daemon(){
  pid_t pid = fork();

  if ( pid < 0 )
    server_log(FATAL, "[make_daemon] failed: ");

  /* parent */
  if ( pid > 0 ){
    printf("PID: %d\n", pid);
    exit(0);
  }

  /* child */
  if ( setsid() < 0 )
    server_log(FATAL, "[make_daemon] setsid() failed: ");

  if ( chdir("/") != 0)
    server_log(FATAL, "[make_daemon] chdir() failed: ");

  close(STDIN_FILENO);
  close(STDERR_FILENO);
  close(STDOUT_FILENO);
  
}


int main(int argc, char *argv[]){
  /* TODO: put this junk in a function in another file */
  init_server();
  DEBUG_ENABLED = 0;
  MY_SIG_CODE=0;

  /*
  if ( argc == 2 ){
    if ( parse_file(argv[1]) != 0 )
      server_log(FATAL, "Could not read config file!!!");
  }else{
  */
    if ( parse_file("./default.conf") != 0 )
      server_log(FATAL, "Could not read config file!!!");
  /*
  }
  */

  if ( SERVER.max_x == 0 || SERVER.max_x < 10 )
    SERVER.max_x =  80;
  if ( SERVER.max_y == 0 || SERVER.max_y < 10 )
    SERVER.max_y =  SERVER.max_x/2;

  serv_put_pellet(NULL);  /* initialize pellet position after bounds set*/

  if ( SERVER.port == 0 ) server_log(FATAL, "0 is not a good port friend");

  struct sockaddr_in host_addr;
  int sockfd, new_sockfd;  // Listen on sock_fd, new connection on new_fd
  int yes = 1;

  socklen_t sin_size;
  SERVER.addr = &host_addr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      server_log(FATAL, "creating socket");

  memset(&host_addr, 0, sizeof(host_addr) );

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0)
    server_log(FATAL, "setting socket option SO_REUSEADDR");

  host_addr.sin_family = AF_INET;
  host_addr.sin_addr.s_addr = htons(INADDR_ANY);
  host_addr.sin_port = htons(SERVER.port);

  if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
      server_log(FATAL, "binding to socket");

  if (listen(sockfd, 5) == -1) 
      server_log(FATAL, "listening on socket");

  server_log(INFO, "Listening on %s:%d", inet_ntoa(host_addr.sin_addr), ntohs(host_addr.sin_port));

  if ( pthread_attr_init(&ATTR) != 0)
    server_log(FATAL, "[main] %s:%d Could not init pthread attributes", __FILE__, __LINE__);
  if ( pthread_attr_setdetachstate(&ATTR, PTHREAD_CREATE_DETACHED) != 0 )
    server_log(FATAL, "[main] %s:%d Could not set attributes to detatch", __FILE__, __LINE__);

  sin_size = sizeof(struct sockaddr_in);

  check_privs();
  if ( SERVER.log != stderr )
    make_daemon();

  player * play = NULL;

  sigset_t signal_mask;
  sigemptyset (&signal_mask);
  sigaddset(&signal_mask, SIGUSR1);
  sigaddset(&signal_mask, SIGTERM);
  set_signal();



  while(1) {
    /* if server is full don't go taking more connections... */
    while( serv_full() ) sleep(1);

    /* a new player structure is needed */
    if ( play == NULL )
       play = init_player();

    new_sockfd = accept(sockfd, (struct sockaddr *)&play->addr, &sin_size);
    if(new_sockfd < 0 ){
      if ( errno == EINTR ){
        if ( MY_SIG_CODE == SIG_CODE_DISPLAY ){
          MY_SIG_CODE = 0;
          /* TODO: call some code here to output information about the game */
        }
        break; /* caught some signal, should stop accepting connections */
      }
      server_log(FATAL, "[main] accepting connection");
    }

    server_log(INFO, "New connection from %s:%d", inet_ntoa(play->addr.sin_addr), ntohs(play->addr.sin_port));
    play->fd = new_sockfd;

    /* set a timeout on socket */
    player_set_timeout(play, 5);

    /* while() sleep() loop should keep this from failing so if it does die */
    if ( serv_add_player(play) )
      server_log(FATAL, "FAILED to add player to server list");

    /* TODO: make this continue and not FATAL? */
    if (pthread_mutex_init(&play->lock, NULL) != 0)
        server_log(FATAL, "mutex init failed");


    if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0)
      server_log(FATAL, "%s:%d [main] failed to set sigmask for thread" , __FILE__, __LINE__);
      
    if ( pthread_create(&play->tid_progress, &ATTR, (void *) &progress_game, play) != 0 )
      server_log(FATAL, "failed to make progress thread");

    if (pthread_sigmask(SIG_UNBLOCK, &signal_mask, NULL) != 0)
      server_log(FATAL, "%s:%d [main] failed to reset sigmask accept" , __FILE__, __LINE__);

    play = NULL;
  }

  close(sockfd);

  /* wait on players to finish up */
  if ( MY_SIG_CODE == SIG_CODE_SLOW )
    serv_wait_on_players();

  /*
   * TODO: still a lot of SERVER stuff to free 
  */

  /* 
   * TODO: destroy player has gotten messy, should be *lower* level function
   *       for this, I should not have to look up what needs free'd
  */
  if ( play != NULL ){
    free(play->pix);
    free(play);
  }
  /* killall the players */

  pthread_attr_destroy(&ATTR);
  if ( SERVER.log != stderr )
    fclose(SERVER.log);

    
  return 0;
}
#undef  SIG_CODE_KILL_ALL
#undef  SIG_CODE_SLOW
#undef  SIG_CODE_DISPLAY 
