baseobjects := ../deps/sqlite/sqlite3.o \
				   ./core/zmalloc.o \
				   ./core/ini.o ./core/sds.o ./core/util.o ./core/adlist.o \
				   ./core/stack.o ./core/dict.o ./core/frozen.o ./core/debug.o \
				   ./core/sha1.o ./core/crc64.o \
				   ./event/device.o ./event/actor.o \
				   ./script/script.o ./script/util.o ./script/ui.o ./script/db.o ./script/net.o \
				   ./net/anet.o ./net/ae.o \
				   ./net/networking.o ./net/resp/resp.o\
				   ./net/resp/service/service.o ./net/resp/service/test.o ./net/resp/service/script.o \
				   ./ui/ui.o ./ui/window.o ./ui/color.o ./ui/map.o ./ui/console.o ./ui/page.o \
				   ./ui/document/document.o ./ui/document/error.o ./ui/document/html.o ./ui/document/css.o \
				   ./ui/document/select.o ./ui/document/render.o ./ui/document/style_computation.o \
				   ./ui/document/style_computation_div.o  ./ui/document/style_computation_extra.o\
				   ./ui/document/style_computation_input.o ./ui/document/style_computation_table.o\
				   ./ui/document/style_computation_text.o
				  

travellerobjects := ./traveller.o $(baseobjects)

uitestobjects := ./uitests/main.o $(baseobjects) ./uitests/ui_test.o

nouitestobjects = ./nouitests/main.o ./nouitests/case.o $(baseobjects) \
				  ./nouitests/core_test.o \
				  ./nouitests/ui_main_test.o \
				  ./nouitests/ui_html_test.o ./nouitests/ui_css_test.o ./nouitests/ui_css_select_test.o \
				  ./nouitests/ui_compute_style_test.o

ALL_INCS= -I ./ \
		  -I ../deps/lua/src/ \
		  -I ../deps/sqlite/ 

STD=-g -Wall
LIBRARY= -lcurses -lpanel ../deps/lua/src/liblua.a 
CFLAGS=$(STD)
CC:=gcc
#CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc') $(CFLAGS)
all:traveller

traveller:$(travellerobjects)
	$(CC) -o traveller $(travellerobjects) $(LIBRARY)  $(CFLAGS)

build_uitest:$(uitestobjects)
	$(CC) -o uitest $(uitestobjects) $(LIBRARY)  $(CFLAGS)

build_nouitest:$(nouitestobjects)
	$(CC) -o nouitest $(nouitestobjects) $(LIBRARY)  $(CFLAGS)

test_noui:build_nouitest
	./nouitest

%.o: %.c 
	$(CC) -static -c $(CFLAGS) $(ALL_INCS) $<  -o $*.o

clean:
	find ./ -name "*.o" | xargs rm
	find ./ -name "*.a" | xargs rm
	rm -f ./traveller ./uitest ./nouitest
