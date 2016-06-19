void show_score(unsigned int score);
void game_over();
void winner();


/*
 * draws borders without overwriting anything between
*/
void draw_board();

/*
 * log stuff
*/
void server_log(char *fmt, ...);

/*
 * die and print message
*/
void fatal(char *msg);
