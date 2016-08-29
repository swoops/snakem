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
 * iterate through a list of items in the config setting values
*/
int iter_param_list(FILE *fp, char *name, int *line);
/*
 * set SERVER params given a line from the config file
*/
int set_params(char *buff, size_t buff_len, size_t line);
/*
 * parses the config file to set SERVER paramaters, returns 0 on success
*/
int parse_file(char *fname);

void parse_add_str(char ***array, size_t *size, size_t max_size, char *value, int line);
