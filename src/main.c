#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include "snake.h"
#include "data_types.h"
#include "menu.h"  /*draw_board()*/
#include <unistd.h> /*get opt*/

void help_menu(char *p, int x){
  fprintf(stderr, "%s: [-x <width> ] [ -y <height> ] \n\n", p);
  fprintf(stderr, "\t-x: \ttotal width of board (default: 50)\n");
  fprintf(stderr, "\t-y: \ttotal height of board (default: width)\n");
  fprintf(stderr, "\t-l: \tlog file\n");
  fprintf(stderr, "\t-e: \terror log file\n");
  fprintf(stderr, "\t-i: \tspeed increase for pellots (default 1000)\n");
  fprintf(stderr, "\n");
  exit(x);
}



int main(int argc, char *argv[]){
  int ch;
  SERVER.max_y = 0;
  SERVER.max_x = 0;
  SERVER.t_inc = 1000;
  SERVER.errorlog = stderr;
  SERVER.log = NULL;

  while ((ch = getopt (argc, argv, "hx:y:i:e:l:")) != -1){
    switch (ch) {
      case 'h':
        help_menu(argv[0], 0);
        break;
      case 'e':
        fprintf(stderr, "not implemented yet\n");
        help_menu(argv[0], 5);
        break;
      case 'l':
        fprintf(stderr, "not implemented yet\n");
        help_menu(argv[0], 5);
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

  // clear screen
  fprintf(FDOUT, "\e[2J");
  draw_board();

  /* make snake */
  player play;
  play.color = 0;
  play.score = 0;
  play.size = 100;
  int a[play.size];
  play.slen = 3;
  int i;
  for (i=play.slen; i>0; i--)
    a[play.slen-i] = i;
  play.pix = a;
  play.dir = RIGHT;
  play.head = 0;

  if (pthread_mutex_init(&play.lock, NULL) != 0) {
      fprintf(FDOUT,"\n mutex init failed\n");
      return 1;
  }

  pthread_t tid;
  if ( pthread_create(&tid, NULL, (void *) &progess_game, &play) != 0 ){
      fprintf(FDOUT,"\n swoopsed it all\n");
      exit(0);
  }

  player_controll(&play);
  return 0;
}
