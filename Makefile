#!/usr/bin/make
VERSION	= 0.0.1
PROG	= tv-i2c-monitor
CC		= /usr/bin/gcc
CFLAGS	= -Wall -Wextra -Wpedantic $(shell pkg-config --cflags dbus-1)
LIB		= -L
LDFLAGS	= $(shell pkg-config --libs dbus-1)

OBJ = tv-i2c-monitor.o

$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< $(LDFLAGS)

clean:
	rm -f $(OBJ) $(PROG)
