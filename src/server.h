/* 
 * initialise server with default values, done before the config, many things
 * need to be freed from this. If a pointer is to potentially later used, but
 * can't be set just yet, set it to NULL here
 */
void init_server();
/* 
 * Go through the server object and free/fclose the objects as needed. Any
 * malloc pointer that is NULL will not be free'd 
 */
void serv_destory();
/* 
 * iterate through players marking them killed, if (wait) then call
 * serv_wait_on_players and sleep till the players are all really gone 
*/
void server_kill_em_all(int wait);

/* block until all players are free'd and gone */
void serv_wait_on_players();

/* simple wrappers for lock/unlock */
void serv_lock();
void serv_unlock();

/* left around in case it is needed again */
#ifdef PLAYER_ARRAY_DBUG
void debug_player_array(char *msg);
#endif

/* check if there is a collision between a point and any alive snake */
int serv_check_collisions(player *p, int head);

/*
 * tell everyone something:
 * color: the color code to use for the font foreground color
 */
void serv_notify_all(int color, char *fmt, ...);

/* is the server full? */
int serv_full();
void serv_put_pellet(player *p);

/*
 * like write(fp, buff, strlen(buff)) but for all the players in SERVER.players
 * that are not dead
 * 
 * it is used by serv_notify_all to do the writing
 */
void serv_write(char *buff);

/* 
 * removes a player from SERVER.players, should only be called by the players
 * thread after the players struct has been cleaned up 
 */
int serv_del_player(player *p);
int serv_add_player(player *p);
int serv_check_highscore(player *p);

/*
 * set mods to randomness and notifies to all players name should be blamed
 * has a 1/chance probability of changing the flags
*/
void serv_random_mods(char *name, int chance);

/* clears all modes that were not set in config */
void serv_clear_mods();

/* server gets */
int serv_get_highscore();
int serv_get_mods();
int serv_get_num_players();
int serv_get_pellet();
