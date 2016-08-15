/* 
 * how many chars are there from buff to the first ' ', '\t'  if '\x00' is hit
 * 0 is returned
*/
size_t sp_strlen(char *buff);
/*
 * return pointer to first character that is not '\t' or ' ' may point to a
 * NULL
*/
char * skip_left_pad(char * buff);
/*
 * set SERVER params given a line from the config file
*/
int set_params(char *buff, size_t buff_len, size_t line);
/*
 * parses the config file to set SERVER paramaters, returns 0 on success
*/
int parse_file(char *fname);
