#include  <stdio.h>
#include  <stdlib.h>
#include  <pthread.h>
#include  "snake.h"
#include  "data_types.h"
#include  <unistd.h>      /*getopt*/
#include  <string.h>     /*memset*/
#include  <arpa/inet.h>   /*htons*/
#include  <signal.h>      /*sigemptyset,sigaction*/
#include  <sys/socket.h>
#include  "player.h"
#include  "logging.h"
#include  "server.h"

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

void help_menu(char *p, int x){
  fprintf(stderr, "%s: [-x <width> ] [ -y <height> ] \n\n", p);
  fprintf(stderr, "\t-x: \ttotal width of board (default: 50)\n");
  fprintf(stderr, "\t-y: \ttotal height of board (default: width)\n");
  fprintf(stderr, "\t-l: \tlog file\n");
  fprintf(stderr, "\t-i: \tspeed increase for pellots (default 1000)\n");
  fprintf(stderr, "\t-s: \tHigh Score to start with... better logging later :)\n");
  fprintf(stderr, "\t-n: \tName of player with high score (default Nobody)\n");
  fprintf(stderr, "\t-b: \tFile to be printed as the start banner\n");
  fprintf(stderr, "\t-m: \tMax number of players\n");
  fprintf(stderr, "\n");
  exit(x);
}

int main(int argc, char *argv[]){
  /* TODO: put this junk in a function in another file */
  int ch;
  init_server();

  while ((ch = getopt (argc, argv, "hp:x:y:b:i:e:l:s:n:")) != -1){
    switch (ch) {
      case 'h':
        help_menu(argv[0], 0);
        break;
      case 'e':
        fprintf(stderr, "not implemented yet\n");
        help_menu(argv[0], 5);
        break;
      case 'b':
        SERVER.start_banner = optarg;
        break;
      case 'p':
        SERVER.port = atoi(optarg);
        break;
      case 'm':
        SERVER.max_players = atoi(optarg);
        break;
      case 's':
        SERVER.high_score = atoi(optarg);
        break;
      case 'n':
        if ( strlen(optarg) > MAX_PLAYER_NAME )
          server_log(FATAL, "%s [main] line: %d provided name too big", __FILE__, __LINE__);

        free(SERVER.hs_name);

        if ( ( SERVER.hs_name = strdup(optarg) ) == NULL )
          server_log(FATAL, "%s [main] line: %d strdup() failed", __FILE__, __LINE__);
        break;
      case 'l':
        if ( ( SERVER.log = fopen(optarg, "a") ) == NULL ){
          server_log(FATAL, "%s [main] line: %d could not open log file", __FILE__, __LINE__);
          help_menu(argv[0], 1);
        }
        break;
      case 'i':
        /* TODO: check bounds */
        SERVER.t_inc = atoi(optarg);
        break;
      case 'x':
        SERVER.max_x = atoi(optarg);
        if ( SERVER.max_x < 10 || SERVER.max_x > 500 ){
          fprintf(stderr, "[!] -x not in range\n");
          help_menu(argv[0], 1);
        }
        break;
      case 'y':
        SERVER.max_y = atoi(optarg);
        if ( SERVER.max_y < 10 || SERVER.max_y > 500 ){
          fprintf(stderr, "[!] -y not in range\n");
          help_menu(argv[0], 1);
        }
        break;
      case '?':
        fprintf(stderr, "misunderstood params\n");
        help_menu(argv[0], 1);
        break;
      default:
        fprintf(stderr, "dont understand something\n");
        help_menu(argv[0], 2);
        break;
    }
  }


  if ( SERVER.max_x == 0 || SERVER.max_x < 10 )
    SERVER.max_x =  50;
  if ( SERVER.max_y == 0 || SERVER.max_y < 10 )
    SERVER.max_y =  SERVER.max_x/2;

  serv_put_pellet(NULL);  /* initialize pellet position after bounds set*/

  if ( SERVER.port == 0 ) server_log(FATAL, "Need a port to bind on");

  struct sockaddr_in host_addr, client_addr;
  int sockfd, new_sockfd;  // Listen on sock_fd, new connection on new_fd

  change_signal();
  socklen_t sin_size;
  
  SERVER.addr = &host_addr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      server_log(FATAL, "creating socket");

  memset(&host_addr, 0, sizeof(host_addr) );

  host_addr.sin_family = AF_INET;
  host_addr.sin_addr.s_addr = htons(INADDR_ANY);
  host_addr.sin_port = htons(SERVER.port);

  if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
      server_log(FATAL, "binding to socket");

  if (listen(sockfd, 5) == -1) 
      server_log(FATAL, "listening on socket");

  server_log(INFO, "Listening on %s:%d", inet_ntoa(host_addr.sin_addr), ntohs(host_addr.sin_port));

  while(1) {
    /* if server is full don't go taking more connections... */
    while( serv_full() ) sleep(1);

    sin_size = sizeof(struct sockaddr_in);
    new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if(new_sockfd == -1 )
      server_log(FATAL, "[main] accepting connection");

    server_log(INFO, "New connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    player * play = init_player();
    play->fd = new_sockfd;
    play->addr = &client_addr;

    /* while() sleep() loop should keep this from failing so if it does die */
    if ( serv_add_player(play) )
      server_log(FATAL, "FAILED to add player to server list");

    /* TODO: make this continue and not FATAL? */
    if (pthread_mutex_init(&play->lock, NULL) != 0)
        server_log(FATAL, "mutex init failed");

    if ( pthread_create(&play->tid_progress, NULL, (void *) &progress_game, play) != 0 )
        server_log(FATAL, "failed to make progress thread");
  }

  if ( SERVER.log != stderr )
    fclose(SERVER.log);
    
  return 0;
}
