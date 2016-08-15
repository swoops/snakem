#include  <stdio.h>
#include  <stdlib.h>
#include  <pthread.h>
#include  "snake.h"
#include  <string.h>      /*memset*/
#include  <arpa/inet.h>   /*htons*/
#include  <signal.h>      /*sigemptyset,sigaction*/
#include  <sys/socket.h>
#include  "player.h"
#include  "logging.h"
#include  "server.h"
#include  "parse_config.h"
#include  "data_types.h"

void change_signal(){
  /**
   * stop sigpipe signal so that write returns error instead of fault
   */
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGPIPE, &sa, 0) == -1) 
    server_log(FATAL, "Could not change SIGPIPE singal");
}


int main(int argc, char *argv[]){
  /* TODO: put this junk in a function in another file */
  init_server();
  DEBUG_ENABLED = 0;

  if ( argc == 2 ){
    if ( parse_file(argv[1]) != 0 )
      server_log(FATAL, "Could not read config file!!!");
  }else{
    if ( parse_file("./default.conf") != 0 )
      server_log(FATAL, "Could not read config file!!!");
  }

  if ( SERVER.max_x == 0 || SERVER.max_x < 10 )
    SERVER.max_x =  80;
  if ( SERVER.max_y == 0 || SERVER.max_y < 10 )
    SERVER.max_y =  SERVER.max_x/2;

  serv_put_pellet(NULL);  /* initialize pellet position after bounds set*/

  if ( SERVER.port == 0 ) server_log(FATAL, "0 is not a good port friend");

  struct sockaddr_in host_addr;
  int sockfd, new_sockfd;  // Listen on sock_fd, new connection on new_fd
  int yes = 1;

  change_signal();
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

  while(1) {
    /* if server is full don't go taking more connections... */
    while( serv_full() ) sleep(1);
    player * play = init_player();

    sin_size = sizeof(struct sockaddr_in);
    new_sockfd = accept(sockfd, (struct sockaddr *)&play->addr, &sin_size);
    if(new_sockfd == -1 )
      server_log(FATAL, "[main] accepting connection");

    server_log(INFO, "New connection from %s:%d", inet_ntoa(play->addr.sin_addr), ntohs(play->addr.sin_port));
    play->fd = new_sockfd;

    /* while() sleep() loop should keep this from failing so if it does die */
    if ( serv_add_player(play) )
      server_log(FATAL, "FAILED to add player to server list");

    /* TODO: make this continue and not FATAL? */
    if (pthread_mutex_init(&play->lock, NULL) != 0)
        server_log(FATAL, "mutex init failed");

    if ( pthread_create(&play->tid_progress, &ATTR, (void *) &progress_game, play) != 0 )
        server_log(FATAL, "failed to make progress thread");
  }

  pthread_attr_destroy(&ATTR);
  if ( SERVER.log != stderr )
    fclose(SERVER.log);
    
  return 0;
}
