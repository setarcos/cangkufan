CC = gcc
CFLAGS = -Wall
OBJS = gfx.o levels.o moves.o

all:	cangkufan

cangkufan:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) `pkg-config --libs gtk+-2.0`

clean:
	rm -f *.o
	rm -f cangkufan

.c.o:
	$(CC) $(CFLAGS) -c `pkg-config --cflags gtk+-2.0` $<

gfx.o:		gfx.c sokoban.h
moves.o: moves.c sokoban.h
levels.o:	levels.c sokoban.h
