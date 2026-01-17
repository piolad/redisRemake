# Variables
CC = gcc
CFLAGS = -Wall -g -Wextra -O2
LDFLAGS = -lm

all: client server

client: client.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

server: server.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

client.o: client.c
	$(CC) $(CFLAGS) -c -o $@ $<

server.o: server.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f client server *.o
