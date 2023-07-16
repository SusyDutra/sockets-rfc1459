# Makefile for IRC Chat Project

# Compiler
CXX := g++

# Flags
CXXFLAGS := -std=c++11 -Wall -pthread

# Source files
SERVER_SRC := server.cpp
CLIENT_SRC := client.cpp

# Output files
SERVER_OUT := server
CLIENT_OUT := client

all: $(SERVER_OUT) $(CLIENT_OUT)

$(SERVER_OUT): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(CLIENT_OUT): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o server client
