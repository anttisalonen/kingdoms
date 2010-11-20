main:
	mkdir -p bin
	g++ -Wall $(CFLAGS) $(shell pkg-config --cflags SDL_image) $(shell pkg-config --libs SDL_image) -o bin/main src/main.cpp

clean:
	rm -rf bin
