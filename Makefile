CC=gcc

ifeq ($(BUILD),debug)
# Debug mode: Unoptimized and with debugging symbols
CFLAGS = -Wall -O0 -g
LFLAGS = 
else
	ifeq ($(BUILD),profile)
	# Profile mode: Debugging symbols and profiling information.
	CFLAGS = -Wall -O0 -pg
	LFLAGS = -pg
	else
	# Release mode: Optimized and stripped of debugging info
	CFLAGS = -Wall -Os -DNDEBUG
	LFLAGS = -s 
	endif
endif

CFLAGS += `sdl-config --cflags`

# -lopengl32 is required for Windows
#LFLAGS += `sdl-config --libs` -lopengl32
LFLAGS += `sdl-config --libs` -lGL

SOURCES= bmp.c game.c ini.c utils.c pak.c particles.c \
	states.c demo.c resources.c musl.c mustate.c hash.c \
	lexer.c

FONTS = fonts/bold.xbm fonts/circuit.xbm fonts/hand.xbm \
		fonts/normal.xbm fonts/small.xbm fonts/smallinv.xbm fonts/thick.xbm

OBJECTS=$(SOURCES:.c=.o)
all: game editor

debug:
	make "BUILD=debug"
	
profile:
	make "BUILD=profile"

game: $(OBJECTS) bin
	$(CC) -o bin/$@ $(OBJECTS) $(LFLAGS) 
	
bin:
	mkdir $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

bmp.o : bmp.h $(FONTS)

game.o : bmp.h ini.h game.h particles.h utils.h states.h resources.h

particles.o : bmp.h

ini.o : utils.h

pak.o : pak.h

states.o : states.h bmp.h ini.h utils.h game.h particles.h resources.h

resources.o : pak.h bmp.h ini.h game.h utils.h hash.h

demo.o : states.h bmp.h game.h resources.h

musl.o : musl.h

mustate.o : bmp.h states.h musl.h game.h ini.h resources.h utils.h

lexer.c : lexer.h

###############################################

fltk-config = fltk-config

CPPFLAGS = `$(fltk-config) --cxxflags` -c -I .
LPPFLAGS = `$(fltk-config) --ldflags` 

# Link with static libstdc++, otherwise you need to have
# libstdc++-6.dll around.
LPPFLAGS += -static-libstdc++

.cpp.o:
	g++ -c $(CPPFLAGS) $< -o $@

editor: editor.o BMCanvas.o LevelCanvas.o TileCanvas.o bmp.o tileset.o map.o lexer.o json.o hash.o utils.o
	g++ -o bin/$@  $^ $(LPPFLAGS)
	
editor.o: editor/editor.cpp
	g++ -c $(CPPFLAGS) $< -o $@

BMCanvas.o: editor/BMCanvas.cpp 
	g++ -c $(CPPFLAGS) $< -o $@

LevelCanvas.o: editor/LevelCanvas.cpp 
	g++ -c $(CPPFLAGS) $< -o $@

TileCanvas.o: editor/TileCanvas.cpp 
	g++ -c $(CPPFLAGS) $< -o $@


###############################################

.PHONY : clean

clean:
	-rm -rf bin/game bin/game.exe bin/editor bin/editor.exe
	-rm -rf *.o 
	-rm -rf *~ gmon.out