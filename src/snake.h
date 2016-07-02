#include "data_types.h"

/*
 * check if col pixel is in the snake body
*/

int snake_collision(player *p, int col);
void move_snake(player *p);
void grow_snake(player *p);
/*
 * thread to progress the game along
*/
int progress_game(player *p);
/*
 * thread for user to control snake direction 
*/
int snake_control(player *p);
/* 
 * put snake on board the first time
*/
void draw_snake(player *p);
/*
 * simpler way to change direction, takes care of lock for you
*/
void change_dir(player *p, unsigned  int dir);
