CXX      ?= g++
CXXFLAGS ?= -O2
CXXFLAGS += -Wall
CXXFLAGS += -DPREFIX=$(PREFIX)
LDFLAGS  ?= -lSDL -lSDL_image -lSDL_ttf -lboost_serialization

PREFIX        ?= /usr/local
INSTALLBINDIR  = $(PREFIX)/bin
SHAREDIR       = $(PREFIX)/share/kingdoms
GFXDIR         = $(SHAREDIR)/gfx
RULESDIR       = $(SHAREDIR)/rules

BINDIR  = bin
BINNAME = kingdoms
TARGET = $(BINDIR)/$(BINNAME)
SRCDIR = src
TMPDIR = tmp
SRCFILES = color.cpp sdl-utils.cpp utils.cpp rect.cpp \
	   resource_configuration.cpp advance.cpp \
	   city_improvement.cpp unit.cpp \
	   city.cpp map.cpp fog_of_war.cpp \
	   government.cpp civ.cpp \
	   pompelmous.cpp \
	   serialize.cpp \
	   astar.cpp map-astar.cpp ai-orders.cpp ai-objective.cpp \
	   ai-debug.cpp ai-exploration.cpp ai-expansion.cpp \
	   ai-defense.cpp ai-offense.cpp ai-commerce.cpp \
	   ai.cpp \
	   gui-utils.cpp city_window.cpp \
	   production_window.cpp \
	   discovery_window.cpp diplomacy_window.cpp main_window.cpp gui.cpp \
	   main.cpp
OBJS   = $(SRCS:.cpp=.o)
DEPS   = $(SRCS:.cpp=.dep)

SRCS = $(addprefix $(SRCDIR)/, $(SRCFILES))

.PHONY: clean all

all: $(BINDIR) $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET)

%.dep: %.cpp
	@rm -f $@
	@$(CC) -MM $(CPPFLAGS) $< > $@.P
	@sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $*)\1.o $@ : ,g' < $@.P > $@
	@rm -f $@.P

install: $(TARGET)
	install -d $(INSTALLBINDIR) $(GFXDIR) $(RULESDIR)
	install -m 0755 $(TARGET) $(INSTALLBINDIR)
	install -m 0644 share/gfx/* $(GFXDIR)
	install -m 0644 share/rules/* $(RULESDIR)

uninstall:
	rm -rf $(INSTALLBINDIR)/$(BINNAME)
	rm -rf $(SHAREDIR)

$(TMPDIR):
	mkdir -p $(TMPDIR)

discoveries: $(TMPDIR)
	cat share/discoveries.txt | runhaskell utils/dot.hs | dot -Tpng > $(TMPDIR)/graph.png

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
	rm -rf $(BINDIR)

-include $(DEPS)

