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

#define CLEAR_SCREEN "\e[2J"

/* for snake flags */
#define DEAD 1    /* snake is dead, clean him off the road */
#define TECHNO_TIAM 2  /*turn techno mode on/off*/



typedef struct {
  int x,y;
} point;

/* should be common to all players, global */
typedef struct {
  int max_x, max_y;
  FILE *log;
  int t_inc;   // how much to increase the speed per pellot
  int port;   // port to listen on
  int high_score;
  struct sockaddr_in * addr;
} server;

/* everying in here is unique to each playser */
typedef struct {
  int *pix;             /* pointer to int array of pixels */
  int head;             /* index of head */
  size_t dir;           /* direction of snake */
  unsigned int slen;    /* how many pixels */
  unsigned int size;    /* total size of array */
  unsigned int score;   /* what do you think it is */
  int pellet;           /* location of pellet*/
  int color;            /* techo snake tiam */
  int flags;            /* sanke attributes */
  useconds_t delay;
  pthread_mutex_t lock; /*lock for threading */
  pthread_t tid_progress;
  pthread_t tid_controll;

  int fd;               /* file descriptor for socket */
  struct sockaddr_in * addr; /* clients address */
} player;


// global server
server SERVER;
#endif
