main:
	mkdir -p bin
	g++ -lSDL -o bin/main src/main.cpp

clean:
	rm -rf bin
