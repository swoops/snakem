#ifndef  data_check
#define data_check
/* for the size of sprintf stuff */
#define MAX_BUFF_SIZE 1024
#define FDOUT stdout  
#include <unistd.h> /*usleep*/
#include<pthread.h> /* mutex_t */
#include<stdio.h> /* mutex_t */
#include <sys/socket.h>

#define  UP     1   //  \e[1A
#define  DOWN   2   //  \e[1B
#define  RIGHT  4   //  \e[1C
#define  LEFT   8   //  \e[1D
#define  HOLDD  16  //  \e[1D

/* for snake flags */
#define DEAD 1    /* snake is dead, clean him off the road */
#define KILL 2    /* snake is dead, clean him off the road */
#define TECHNO_TIAM 4  /*turn techno mode on/off*/


#define MAX_PLAYER_NAME 16 


/* char codes usesd often */

#define GO_HOME_STR "\e[H"
#define CLEAR_SCREEN_STR "\e[2J"



typedef struct {
  int x,y;
} point;


/* everying in here is unique to each playser */
typedef struct {
  char *name;                   /* username for the snake */
  int *pix;                     /* pointer to int array of pixels */
  int head;                     /* index of head */
  size_t dir;                   /* direction of snake */
  unsigned int slen;            /* how many pixels */
  unsigned int size;            /* total size of array */
  unsigned int score;           /* what do you think it is */
  int color;                    /* snake color */
  int flags;                    /* sanke attributes */
  useconds_t delay;             /* time to wait between advance */
  pthread_mutex_t lock;         /* lock for threading */
  pthread_t tid_progress;
  pthread_t tid_controll;
  int fd;                       /* file descriptor for socket */
  struct sockaddr_in * addr;    /* clients address */
} player;

/* should be common to all players, global */
typedef struct {
  int max_x, max_y;
  FILE *log;
  int t_inc;                    /* how much to increase the speed per pellot */
  int port;                     /* port to listen on */
  int pellet;                   /* location of pellet*/
  unsigned int high_score;
  struct sockaddr_in * addr;
  char * start_banner;
  player ** players;            /* array of players NULL terminated */
  int max_players;              /* max number of players */
  int last_player;              /* index of last player */
  pthread_mutex_t lock;         /* lock for threading */
  char * hs_name;               /* name of player with high score */

} server;


// global server
server SERVER;
#endif
