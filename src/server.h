void serv_notify_all(int color, char *fmt, ...);
int serv_full();
void serv_put_pellet(player *p);
void serv_write(char *buff);
void init_server();

void serv_lock();
void serv_unlock();

int serv_del_player(player *p);
int serv_add_player(player *p);
int serv_check_highscore(player *p);


/* 
 * iterate through players marking them killed, if (wait) then call
 * serv_wait_on_players and sleep till the players are all really gone 
*/
void server_kill_em_all(int wait);

/* block until all players are gone */
void serv_wait_on_players();

int serv_random_flags();
void serv_set_flags(int flags);

/* server gets */
int serv_get_highscore();
int serv_get_flags();
int serv_get_num_players();
int serv_get_pellet();

/* check if there is a collision between a point and any alive snake */
int serv_check_collisions(player *p, int head);
