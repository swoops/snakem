CC = gcc

# CFLAGS = -Wall
CFLAGS = -Wall -g3 -rdynamic

LIBRARIES = -lpthread
TARGET = snakem

all: $(TARGET)

$(TARGET): movement.o menu.o snake.o  main.c 
	$(CC) $(CFLAGS) $(LIBRARIES) server.o logging.o player.o movement.o menu.o snake.o -o ../$(TARGET) main.c

movement.o: movement.c 
	$(CC) $(CFLAGS) -c movement.c 

logging.o: logging.c 
	$(CC) $(CFLAGS) -c logging.c 

player.o:  player.c logging.o server.o
	$(CC) $(CFLAGS) -c player.c 

server.o:  server.h server.c logging.o 
	$(CC) $(CFLAGS) -c server.c

menu.o:  menu.c movement.o player.o
	$(CC) $(CFLAGS) -c menu.c 

snake.o:  snake.c snake.h menu.o movement.o  player.o logging.o
	$(CC) $(CFLAGS) -c snake.c 

# install: $(TARGET)
# 	install -o root -g root -m 755 $(TARGET) /opt/$(TARGET)
# 
# uninstall: 
# 	rm /opt/$(TARGET)

clean: 
	rm ../$(TARGET) server.o movement.o menu.o snake.o player.o logging.o

