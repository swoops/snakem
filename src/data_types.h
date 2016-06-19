#ifndef  data_check
#define data_check
#define FDOUT stdout  
#include <unistd.h> /*usleep*/
#include<pthread.h> /* mutex_t */

#define  UP     1   //  \e[1A
#define  DOWN   2   //  \e[1B
#define  RIGHT  4   //  \e[1C
#define  LEFT   8   //  \e[1D
#define  HOLDD  16  //  \e[1D


typedef struct {
  int x,y;
} point;

/* should be common to all players, global */
typedef struct {
  int max_x, max_y;
  FILE *errorlog;
  FILE *log;
  int t_inc;   // how much to increase the speed per pellot
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
  useconds_t delay;
  pthread_mutex_t lock; /*lock for threading */
} player;


// global server
server SERVER;
#endif
