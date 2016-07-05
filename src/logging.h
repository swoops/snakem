#define INFO    0
#define DEBUG   2
#define ERROR   4
#define FATAL   8

/*
 * log stuff
*/
void server_log(int flags, char *fmt, ...);
void hexdump(char *str);
