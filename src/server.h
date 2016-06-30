void serv_write(char *buff);
int serv_get_highscore();
void init_server();
void serv_lock();
void serv_unlock();
int serv_del_player(player *p);
int serv_add_player(player *p);
void serv_check_highscore(player *p);
