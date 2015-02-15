travellerobjects = ./core/traveller.o \
				   ./core/zmalloc.o \
				   ./core/config.o \
				   ./core/sds.o \
				   ./core/util.o \
				   ./core/adlist.o \
				   ./core/dict.o \
				   ./net/anet.o \
				   ./net/ae.o \
				   ./net/networking.o \
				   ./netcmd/netcmd.o \
				   ./netcmd/test.o \
				   ./netcmd/plannet.o \
				   ./plannet/plannet.o \
				   ./plannet/db.o \
				   ./plannet/net.o \
				   ../deps/sqlite/sqlite3.o 


ALL_INCS= -I ./ \
		  -I ../deps/lua/src/ \
		  -I ../deps/sqlite/ 

STD=-g -Wall
LIBRARY=-lncurses ../deps/lua/src/liblua.a 
CFLAGS=$(STD)
CC:=gcc
#CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc') $(CFLAGS)
all:traveller

traveller:$(travellerobjects)
	$(CC) -o traveller $(travellerobjects) $(LIBRARY)

%.o: %.c 
	$(CC) -c $(CFLAGS) $(ALL_INCS) $<  -o $*.o

clean:
	rm ./traveller
	rm -f ./core/*.o ./core/*.a
	rm -f ./net/*.o ./net/*.a
	rm -f ./netcmd/*.o ./netcmd/*.a
	rm -f ./plannet/*.o ./plannet/*.a
