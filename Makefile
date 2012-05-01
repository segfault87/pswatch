CC=gcc
CFLAGS=-Wall -O2

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

watcher: context.o log.o pswatch.o main.o
	$(CC) -o watcher context.o log.o pswatch.o main.o

all: watcher
