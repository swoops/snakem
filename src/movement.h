void clear_screen(player *p);
/*
 * sends cursor to origin, and changes the cursor position
 *
*/
void go_home_cursor(player *p);

/*
 * go to (x,y) and print this string because you could print console codes with
 * this you can't be sure where the cursor is after, thus go_home() is called
 * to fix cursor location.
*/
void place_str(int x, int y, player *p, char *fmt, ...) ;

/*
 * helper to make sure a number is in the play field before sending the cursor
 * there
*/
int check_bounds(int n, int max, int min);
int num_to_cord(int num, point *p);
int cord_to_num(point *p);
