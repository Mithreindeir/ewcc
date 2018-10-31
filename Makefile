CC = gcc
EXE = rcc
SOURCES = $(wildcard src/*.c src/frontend/*.c)
OBJS = $(SOURCES:.c=.o)
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	CXXFLAGS = -std=c99 -Wall -Wextra -pedantic -ggdb3
	CFLAGS = $(CXXFLAGS)
endif

.c.o:
	$(CC) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(EXE) $(OBJS)
