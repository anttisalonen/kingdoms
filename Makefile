CXX      ?= g++
CXXFLAGS ?= -O2
CXXFLAGS += -Wall
CXXFLAGS += -DPREFIX=$(PREFIX)
LDFLAGS  += -lSDLmain -lSDL -lSDL_image -lSDL_ttf -lboost_system -lboost_serialization -lboost_filesystem -lboost_iostreams

PREFIX        ?= /usr/local
INSTALLBINDIR  = $(PREFIX)/bin
SHAREDIR       = $(PREFIX)/share/kingdoms
GFXDIR         = $(SHAREDIR)/gfx
RULESDIR       = $(SHAREDIR)/rules

BINDIR  = bin
KINGDOMSNAME = kingdoms
EDITORNAME = kingdoms-mapedit
KINGDOMS = $(BINDIR)/$(KINGDOMSNAME)
EDITOR   = $(BINDIR)/$(EDITORNAME)

SRCDIR = src
TMPDIR = tmp

LIBKINGDOMSSRCFILES = color.cpp sdl-utils.cpp utils.cpp rect.cpp \
	   resource_configuration.cpp resource.cpp advance.cpp \
	   city_improvement.cpp unit.cpp \
	   city.cpp map.cpp fog_of_war.cpp \
	   government.cpp civ.cpp \
	   pompelmous.cpp \
	   serialize.cpp \
	   filesystem.cpp \
	   astar.cpp map-astar.cpp \
	   parse_rules.cpp

LIBKINGDOMSSRCS = $(addprefix $(SRCDIR)/, $(LIBKINGDOMSSRCFILES))
LIBKINGDOMSOBJS = $(LIBKINGDOMSSRCS:.cpp=.o)
LIBKINGDOMSDEPS = $(LIBKINGDOMSSRCS:.cpp=.dep)

LIBKINGDOMS = libkingdoms.a

KINGDOMSSRCFILES = ai-orders.cpp ai-objective.cpp \
	   ai-debug.cpp ai-exploration.cpp ai-expansion.cpp \
	   ai-defense.cpp ai-offense.cpp ai-commerce.cpp \
	   ai.cpp \
	   gui-utils.cpp city_window.cpp \
	   production_window.cpp \
	   relationships_window.cpp \
	   discovery_window.cpp diplomacy_window.cpp \
	   main_window.cpp game_window.cpp \
	   mapview.cpp gui-resources.cpp gui.cpp \
	   main.cpp

KINGDOMSSRCS = $(addprefix $(SRCDIR)/, $(KINGDOMSSRCFILES))
KINGDOMSOBJS = $(KINGDOMSSRCS:.cpp=.o)
KINGDOMSDEPS = $(KINGDOMSSRCS:.cpp=.dep)

EDITORSRCFILES = gui-utils.cpp gui-resources.cpp \
		 main_window.cpp editor_window.cpp editorgui.cpp mapview.cpp \
		 mapedit.cpp

EDITORSRCS = $(addprefix $(SRCDIR)/, $(EDITORSRCFILES))
EDITOROBJS = $(EDITORSRCS:.cpp=.o)
EDITORDEPS = $(EDITORSRCS:.cpp=.dep)

.PHONY: clean all

all: $(KINGDOMS) $(EDITOR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(LIBKINGDOMS): $(LIBKINGDOMSOBJS)
	$(AR) rcs $(LIBKINGDOMS) $(LIBKINGDOMSOBJS)

$(KINGDOMS): $(BINDIR) $(LIBKINGDOMS) $(KINGDOMSOBJS) 
	$(CXX) $(LDFLAGS) $(KINGDOMSOBJS) $(LIBKINGDOMS) -o $(KINGDOMS)

$(EDITOR): $(BINDIR) $(LIBKINGDOMS) $(EDITOROBJS)
	$(CXX) $(LDFLAGS) $(EDITOROBJS) $(LIBKINGDOMS) -o $(EDITOR)

%.dep: %.cpp
	@rm -f $@
	@$(CC) -MM $(CPPFLAGS) $< > $@.P
	@sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $*)\1.o $@ : ,g' < $@.P > $@
	@rm -f $@.P

install: $(KINGDOMS) $(EDITOR)
	install -d $(INSTALLBINDIR) $(GFXDIR) $(RULESDIR)
	install -m 0755 $(KINGDOMS) $(INSTALLBINDIR)
	install -m 0755 $(EDITOR) $(INSTALLBINDIR)
	install -m 0644 share/gfx/* $(GFXDIR)
	install -m 0644 share/rules/* $(RULESDIR)

uninstall:
	rm -rf $(INSTALLBINDIR)/$(KINGDOMSNAME)
	rm -rf $(INSTALLBINDIR)/$(EDITORNAME)
	rm -rf $(SHAREDIR)

$(TMPDIR):
	mkdir -p $(TMPDIR)

discoveries: $(TMPDIR)
	cat share/rules/discoveries.txt | runhaskell utils/dot.hs | dot -Tpng > $(TMPDIR)/graph.png

clean:
	rm -f $(SRCDIR)/*.o $(SRCDIR)/*.dep $(LIBKINGDOMS)
	rm -rf $(BINDIR)

-include $(LIBKINGDOMSDEPS)
-include $(KINGDOMSDEPS)
-include $(EDITORDEPS)

