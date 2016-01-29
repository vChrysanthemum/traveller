baseobjects := ../deps/sqlite/sqlite3.o \
				   ./core/zmalloc.o \
				   ./core/ini.o ./core/sds.o ./core/util.o ./core/adlist.o \
				   ./core/dict.o ./core/frozen.o ./core/debug.o \
				   ./core/sha1.o ./core/crc64.o \
				   ./event/device.o ./event/actor.o \
				   ./net/anet.o ./net/ae.o ./net/networking.o \
				   ./script/script.o ./script/util.o \
				   ./script/db.o \
				   ./script/net.o \

travellerobjects := ./traveller.o $(baseobjects) \
				   ./service/service.o ./service/test.o ./service/galaxies.o \
				   ./ui/ui.o \
				   ./ui/window.o ./ui/color.o ./ui/map.o ./ui/console.o ./ui/page.o 

uitestobjects := ./uitests/main.o $(baseobjects) \
					   ./service/service.o ./service/test.o ./service/galaxies.o \
					   ./uitests/ui_test.o \
					   ./ui/window.o ./ui/color.o ./ui/map.o ./ui/console.o ./ui/page.o 

nouitestobjects = ./nouitests/case.o ./nouitests/ui_test.o

ALL_INCS= -I ./ \
		  -I ../deps/lua/src/ \
		  -I ../deps/sqlite/ 

STD=-g -Wall
LIBRARY= -lcurses /usr/local/lib/libpanel.a ../deps/lua/src/liblua.a 
CFLAGS=$(STD)
CC:=gcc
#CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc') $(CFLAGS)
all:traveller

traveller:$(travellerobjects)
	$(CC) -o traveller $(travellerobjects) $(LIBRARY)  $(CFLAGS)

uitest:$(uitestobjects)
	$(CC) -o uitest $(uitestobjects) $(LIBRARY)  $(CFLAGS)

nouitest:$(nouitestobjects)
	$(CC) -o nouitest $(nouitestobjects) $(LIBRARY)  $(CFLAGS)

%.o: %.c 
	$(CC) -static -c $(CFLAGS) $(ALL_INCS) $<  -o $*.o

clean:
	rm -f ./*.o ./*.a
	rm -f ./core/*.o
	rm -f ./net/*.o
	rm -f ./ui/*.o
	rm -f ./service/*.o
	rm -f ./event/*.o
	rm -f ./script/*.o
	rm -f ./uitests/*.o
	rm -f ./nouitests/*.o
	rm -f ./traveller ./uitest ./nouitest
