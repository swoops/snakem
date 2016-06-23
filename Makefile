CC = gcc

CFLAGS = -Wall 

LIBRARIES = pthread
TARGET = snakem

all: 
	cd src; make
clean: 
	cd src; make clean

