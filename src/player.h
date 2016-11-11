/* flags for player_get_str */
#define  SHADOW_CHARS   1  /* turn all valid chars into "*" */
#define  MIN_IAC_REQ    2  /* require at least a few IAC chars to show you are using telnet (bot check) */
#define  NO_FLIP_SPACE  4  /* don't s/ /_/  player names don't need spaces, I am paranoid, but msgs probabbly do */

/* getch for a player socket */
char player_getc(player *p);

/*set the timeout in seconds on socket read*/
void player_set_timeout(player *p, time_t sec);

/* 
 * binary search for string needle in the list of illegal player names in the
 * SERVER.bnames.  Bad names original obtained from config file and should be
 * sorted.
*/
int player_bin_bad_name_check(char *needle);

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

int check_spectator(player *p);
void player_is_a_bot(player *);
