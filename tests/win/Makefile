winobjects = win.o zmalloc.o

STD=-g -Wall
LIBRARY=-lncurses
CFLAGS=$(STD)
CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc') $(CFLAGS)
all:win

win:$(winobjects)
	$(CC) -o win $(winobjects) $(LIBRARY)

%.d: %.c 
	$(CC) -MT "$*.o $*.d" -MM $(CFLAGS) $< > $@

clean:
	rm -f *.o *.a win
