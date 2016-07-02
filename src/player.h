size_t player_term_string(char *s);
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
