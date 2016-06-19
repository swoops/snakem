CC = gcc

CFLAGS = -Wall

LIBRARIES = pthread
TARGET = snakem

all: $(TARGET)

$(TARGET): movement.o menu.o snake.o  main.c 
	$(CC) -g $(CFLAGS) -l $(LIBRARIES) movement.o menu.o snake.o -o ../$(TARGET) main.c

movement.o: movement.c 
	$(CC) -c movement.c 

manu.o:  menu.c movement.o
	$(CC) -c menu.c 

snake.o:  snake.c menu.o movement.o debug.h
	$(CC) -c snake.c 

# install: $(TARGET)
# 	install -o root -g root -m 755 $(TARGET) /opt/$(TARGET)
# 
# uninstall: 
# 	rm /opt/$(TARGET)

clean: 
	rm ../$(TARGET) movement.o menu.o snake.o

