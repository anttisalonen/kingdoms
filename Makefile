CFLAGS ?= -Wall
LDFLAGS += -lSDL -lSDL_image -lSDL_ttf
BINDIR = bin/
SRCDIR = src/
SOURCES = $(SRCDIR)main.cpp $(SRCDIR)color.cpp $(SRCDIR)sdl-utils.cpp $(SRCDIR)utils.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = $(BINDIR)main

all: bindir $(SOURCES) $(EXECUTABLE)

bindir:
	mkdir -p $(BINDIR)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) $< -c -o $@

clean:
	rm -rf bin src/*.o

