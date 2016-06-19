#include "data_types.h"
void move_snake(player *p);
void put_pellet(player *p);
void grow_snake(player *p);
/*
 * thread to progress the game along
*/
int progess_game(player *p);
/*
 * thread for user to control snake direction
*/
int player_controll(player *p);
/* 
 * put snake on board the first time
*/
void draw_snake(player *p);
