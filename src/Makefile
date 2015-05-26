travellerobjects = ./core/traveller.o \
				   ./core/zmalloc.o \
				   ./core/config.o \
				   ./core/sds.o \
				   ./core/util.o \
				   ./core/adlist.o \
				   ./core/dict.o \
				   ./core/frozen.o \
				   ./core/debug.o \
				   ./core/sha1.o \
				   ./core/crc64.o \
				   ../deps/sqlite/sqlite3.o \
				   ./net/anet.o \
				   ./net/ae.o \
				   ./net/networking.o \
				   ./netcmd/netcmd.o \
				   ./netcmd/test.o \
				   ./netcmd/planet.o \
				   ./script/script.o \
				   ./script/citizen.o \
				   ./script/planet.o \
				   ./script/db.o \
				   ./script/net.o \
				   ./ui/ui.o \
				   ./ui/map.o 


ALL_INCS= -I ./ \
		  -I ../deps/lua/src/ \
		  -I ../deps/sqlite/ 

STD=-g -Wall
LIBRARY= /usr/local/lib/libncurses.a ../deps/lua/src/liblua.a 
CFLAGS=$(STD)
CC:=gcc
#CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc') $(CFLAGS)
all:traveller

traveller:$(travellerobjects)
	$(CC) -o traveller $(travellerobjects) $(LIBRARY)  $(CFLAGS)

%.o: %.c 
	$(CC) -static -c $(CFLAGS) $(ALL_INCS) $<  -o $*.o

clean:
	rm ./traveller
	rm -f ./core/*.o ./core/*.a
	rm -f ./net/*.o ./net/*.a
	rm -f ./ui/*.o ./ui/*.a
	rm -f ./netcmd/*.o ./netcmd/*.a
	rm -f ./script/*.o ./script/*.a
