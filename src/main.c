#include  <stdio.h>
#include  <stdlib.h>
#include  <pthread.h>
#include  "snake.h"
#include  "data_types.h"
#include  "menu.h"        /*draw_board()*/
#include  <unistd.h>      /*getopt*/
#include  <strings.h>     /*bzero*/
#include <arpa/inet.h> /*htons*/
#include  <sys/socket.h>
#include  "player.h"


void help_menu(char *p, int x){
  fprintf(stderr, "%s: [-x <width> ] [ -y <height> ] \n\n", p);
  fprintf(stderr, "\t-x: \ttotal width of board (default: 50)\n");
  fprintf(stderr, "\t-y: \ttotal height of board (default: width)\n");
  fprintf(stderr, "\t-l: \tlog file\n");
  fprintf(stderr, "\t-i: \tspeed increase for pellots (default 1000)\n");
  fprintf(stderr, "\n");
  exit(x);
}



int main(int argc, char *argv[]){
  /* TODO: put this junk in a function in another file */
  int ch;
  SERVER.max_y = 0;
  SERVER.max_x = 0;
  SERVER.port = 0;
  SERVER.t_inc = 1000;
  SERVER.log = stderr;

  while ((ch = getopt (argc, argv, "hp:x:y:i:e:l:")) != -1){
    switch (ch) {
      case 'h':
        help_menu(argv[0], 0);
        break;
      case 'e':
        fprintf(stderr, "not implemented yet\n");
        help_menu(argv[0], 5);
        break;
      case 'p':
        SERVER.port = atoi(optarg);
        break;
      case 'l':
        if ( ( SERVER.log = fopen(optarg, "a") ) == NULL )
          fatal("could not open log file");
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
    SERVER.max_y =  SERVER.max_x;

  /* playing locally... I am so lonely... */
  if ( SERVER.port == 0 ){
    /* clear screen */
    fprintf(FDOUT, "\e[2J");
    draw_board();

    /* make snake */
    player *play = init_player();

    if (pthread_mutex_init(&play->lock, NULL) != 0) {
        fprintf(FDOUT,"\n mutex init failed\n");
        return 1;
    }

    if ( pthread_create(&player->tid_progress, NULL, (void *) &progess_game, play) != 0 )
        fatal("\n swoopsed it all progress\n");


    if ( pthread_create(&player->tid_controll, NULL, (void *) &player_controll, play) != 0 )
        fatal("\n swoopsed it all controll\n");

    sleep(100); /* TODO:!!!!! fix this, this is soooo stupid */
    fprintf(FDOUT, "\e[2J");
    destory_player(play);
  }else{
    /*
    ** SERVER MODE!!!! **
    */
    struct sockaddr_in host_addr, client_addr;
    int sockfd, new_sockfd;  // Listen on sock_fd, new connection on new_fd
    socklen_t sin_size;
    
    SERVER.addr = &host_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        fatal("creating socket");

    bzero(&host_addr, sizeof(host_addr) );

    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = htons(INADDR_ANY);
    host_addr.sin_port = htons(SERVER.port);

    if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
        fatal("binding to socket");

    if (listen(sockfd, 5) == -1) 
        fatal("listening on socket");

    server_log("Listening on %s:%d\n", inet_ntoa(host_addr.sin_addr), ntohs(host_addr.sin_port));

    while(1) {
      sin_size = sizeof(struct sockaddr_in);
      new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
      if(new_sockfd == -1 ) {
        fatal("accepting connection");
      }

      server_log("New player from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
      player * play = init_player();
      play->fd = new_sockfd;
      play->addr = &client_addr;

      if (pthread_mutex_init(&play->lock, NULL) != 0) {
          fprintf(FDOUT,"\n mutex init failed\n");
          return 1;
      }

      if ( pthread_create(&player->tid_progress, NULL, (void *) &progess_game, play) != 0 )
          fatal("\n swoopsed it all progress\n");

      fprintf(FDOUT, "\e[2J");
      draw_board();

      if ( pthread_create(&player->tid_controll, NULL, (void *) &player_controll, play) != 0 )
          fatal("\n swoopsed it all controll\n");
    }

    server_log("shutting down server\n");
  }
  if ( SERVER.log != stderr )
    fclose(SERVER.log);
    
  return 0;
}
