#
# Makefile for bgrunner
#

CC=gcc
# c99 vs gnu99: c99 generates warnings with usleep, gnu99 doesn't
CFLAGS=-Wall -pedantic -std=gnu99
LDFLAGS=-lpthread -std=gnu99
EXECUTABLE=bgrunner

all: $(EXECUTABLE)

$(EXECUTABLE): bgrunner.o
	$(CC) -o $(EXECUTABLE) bgrunner.c bgrunnerfuncs.c $(LDFLAGS)

clean:
	rm *.o $(EXECUTABLE)

install:
	mkdir -p $(DESTDIR)
	install -m 0755 $(EXECUTABLE) $(DESTDIR)
	install -m 0644 doc/bgrunner.1.gz $(MANDIR)
