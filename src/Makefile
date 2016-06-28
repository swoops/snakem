CC = gcc

CFLAGS = -Wall

LIBRARIES = pthread
TARGET = snakem

all: $(TARGET)

$(TARGET): movement.o menu.o snake.o  main.c 
	$(CC) $(CFLAGS) -l $(LIBRARIES) logging.o player.o movement.o menu.o snake.o -o ../$(TARGET) main.c

movement.o: movement.c 
	$(CC) $(CFALGS) -c movement.c 

logging.o: logging.c 
	$(CC) $(CFALGS) -c logging.c 

player.o:  player.c logging.o 
	$(CC) $(CFLAGS) -c player.c 

menu.o:  menu.c movement.o player.o
	$(CC) $(CFALGS) -c menu.c 

snake.o:  snake.c snake.h menu.o movement.o  player.o logging.o
	$(CC) $(CFALGS) -c snake.c 

# install: $(TARGET)
# 	install -o root -g root -m 755 $(TARGET) /opt/$(TARGET)
# 
# uninstall: 
# 	rm /opt/$(TARGET)

clean: 
	rm ../$(TARGET) movement.o menu.o snake.o player.o logging.o

