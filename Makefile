CC = gcc
CFLAGS = -Wall -Wstrict-prototypes -O2 -pipe
SRC = spicd.c
OBJ = $(SRC:.c=.o)
LIBS =
INCLUDEDIR = /usr/src/linux
PREFIX = /usr/local
DESTBIN = $(PREFIX)/bin
DESTMAN = $(PREFIX)/man/man1

all: spicd

spicd: $(OBJ)
	$(CC) -o spicd $(OBJ) $(LIBS)

clean:
	rm -f $(OBJ) spicd *~

install:
	install -m 755 spicd $(DESTBIN)
	install -m 644 spicd.1 $(DESTMAN)
