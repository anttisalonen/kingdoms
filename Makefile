CFLAGS ?= -g3 -Wall
LDFLAGS += -lSDL -lSDL_image -lSDL_ttf
BINDIR = bin
SRCDIR = src
INCLUDES = $(SRCDIR)/buf2d.h $(SRCDIR)/civ.h $(SRCDIR)/color.h $(SRCDIR)/gui.h $(SRCDIR)/sdl-utils.h $(SRCDIR)/utils.h
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/color.cpp $(SRCDIR)/sdl-utils.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/civ.cpp $(SRCDIR)/gui.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = $(BINDIR)/main

DEPS = make.dep

default: all

$(DEPS): $(SOURCES) $(INCLUDES)
	$(CXX) -MM $(SOURCES) > $(DEPS)

-include $(DEPS)

all: $(EXECUTABLE) $(SOURCES) $(OBJECTS)

bindir:
	mkdir -p $(BINDIR)

$(EXECUTABLE): bindir $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.cpp $(DEPS)
	$(CXX) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BINDIR) $(DEPS) src/*.o

