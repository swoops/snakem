/* 
 * change local user to a uid
 * thanks goes to Matt Joiner for the bassis of this. 
 * https://stackoverflow.com/questions/3836365/how-can-i-get-the-user-id-associated-with-a-login-on-linux
 * Was not my question but I drew from code he provided to clarify how the
 * struct would load the string buffers inside it and proper clean up
 */ 
static uid_t name_to_uid(char const *name);
/* 
 * how many chars are there from buff to the first ' ', '\t'  if '\x00' is hit
 * 0 is returned
*/
static size_t sp_strlen(char *buff);
/*
 * return pointer to first character that is not '\t' or ' ' may point to a
 * NULL
*/
static char * skip_left_pad(char * buff);
/*
 * iterate through a list of items in the config setting values
*/
static void iter_param_list(FILE *fp, char *name, int *line);
/*
 * set SERVER params given a line from the config file
*/
static int set_params(char *buff, size_t buff_len, size_t line);
/* add string to array */
static void parse_add_str(char ***array, size_t *size, size_t max_size, char *value, int line);

/*
 * top down merge sort a char ** containing strings.  
 *     sizeof(old) == sizeof(new) == size 
 * for this to work.  This returns a ptr to either old or new.
 * no buffers are creaetd or removed by this function
*/
static char ** merge_sort_str(char **old, char **new, size_t size);
