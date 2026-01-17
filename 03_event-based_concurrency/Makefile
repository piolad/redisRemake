# Variables
CC     = gcc
CXX    = g++
CFLAGS = -Wall -g -Wextra -O2
CXXFLAGS = -Wall -g -Wextra -O2
LDFLAGS = -lm

all: client server

client: client.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

server: server.o
	$(CXX) $(CFLAGS) -o $@ $< $(LDFLAGS)

client.o: client.c
	$(CC) $(CFLAGS) -c -o $@ $<

server.o: server.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f client server *.o
