#define INFO  0
#define ERROR  1
#define FATAL 2

/*
 * log stuff
*/
void server_log(int flags, char *fmt, ...);
void hexdump(char *str);
