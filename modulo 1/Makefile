# Makefile for IRC Chat Project

# Compiler
CXX := g++

# Flags
CXXFLAGS := -std=c++11 -Wall -pthread

# Source file
CLIENT_SRC := client.cpp

# Output files
CLIENT_OUT := client

all: $(CLIENT_OUT)

$(CLIENT_OUT): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o client
