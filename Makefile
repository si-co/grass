
# https://stackoverflow.com/questions/3220277/what-do-the-makefile-symbols-and-mean

SRCDIR   = src
BINDIR   = bin
INCLUDES = include
LIBDIR   = lib

CC=g++
CFLAGS=-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11 -I $(INCLUDES)/ -m32
DEPS = $(wildcard $(INCLUDES)/%.hpp)


    #
    #  Dependancies
    #

bin/grass.o : src/grass.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

bin/utils.o : lib/utils.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

bin/commands.o : lib/commands.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

bin/parser.o : lib/parser.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

bin/sockutils.o : lib/sockutils.cpp
	$(CC) $(CFLAGS) -c -o $@ $^


    #
    #   Client / server
    #

bin/client: src/client.cpp bin/commands.o bin/parser.o bin/sockutils.o bin/utils.o bin/grass.o
	$(CC) $(CFLAGS) $^ -o $@

bin/server: src/server.cpp bin/commands.o bin/parser.o bin/sockutils.o bin/utils.o bin/grass.o
	$(CC) $(CFLAGS) $^ -o $@

all: bin/client bin/server




    #
    #   Clean
    #

.PHONY: clean
clean:
	rm -f $(BINDIR)/client $(BINDIR)/server
	rm bin/*.o




    #
    #   Old stuff
    #
#LIBFILES:= $(wildcard $(LIBDIR)/*.cpp)
#binS := $(patsubst $(LIBDIR)/%.cpp,$(binDIR)/%.o,$(LIBFILES))
#$(binDIR)/%.o : $(LIBDIR)/%.cpp
#	$(CC) $(CFLAGS) -c -o $@ $<
