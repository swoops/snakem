#define SHADOW_CHARS 1  /* flag for player_get_str */

/* clean up a bot a bot earlier */
void player_kill_bot(player *p);

/* getch for a player socket */
char player_getc(player *p);

/* ask for and set name of snake */
int player_set_name(player *p);

/* mutex lock/unlock */
void player_unlock(player *p);
void player_lock(player *p);

/* destroy player in two phases, one for each thread*/
void destroy_player(player *p);

/* initalise a player */
player * init_player();

int player_write(player *p, char *msg);

/* put chars in buff 1 at a time up to size big */
size_t player_get_str(player *p, char *buff, size_t size, int flags);
