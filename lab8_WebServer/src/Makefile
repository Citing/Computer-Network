all: webserver

CC = clang++

OPTIONS = -Wall -O3 -std=c++17

webserver: webserver.cpp
	$(CC) $(OPTIONS) -o webserver webserver.cpp

clean:
	-rm -f webserver