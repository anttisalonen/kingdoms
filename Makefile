main:
	mkdir -p bin
	g++ -Wall $(CFLAGS) $(shell pkg-config --cflags sdl) $(shell pkg-config --libs sdl) -lSDL_image -lSDL_ttf -o bin/main src/main.cpp

clean:
	rm -rf bin
