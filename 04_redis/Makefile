CXX = g++

CXXFLAGS = -Wall -Wextra -std=c++11

CLIENT_SRC = src/client/main.cpp
SERVER_SRC = src/server/main.cpp

CLIENT_EXEC = client
SERVER_EXEC = server

.PHONY: all clean

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC)

$(SERVER_EXEC): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC)

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)
