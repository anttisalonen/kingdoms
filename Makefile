CXXFLAGS = -Wall -g3
LDFLAGS  = -lSDL -lSDL_image -lSDL_ttf

BINDIR = bin
TARGET = $(BINDIR)/main
SRCDIR = src
SRCS   = $(SRCDIR)/color.cpp $(SRCDIR)/sdl-utils.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/rect.cpp \
	 $(SRCDIR)/civ.cpp \
	 $(SRCDIR)/gui-utils.cpp $(SRCDIR)/main_window.cpp $(SRCDIR)/city_window.cpp $(SRCDIR)/gui.cpp \
	 $(SRCDIR)/main.cpp
OBJS   = $(SRCS:.cpp=.o)
DEPS   = $(SRCS:.cpp=.dep)

.PHONY: clean all

all: $(BINDIR) $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

%.cpp.o: %.dep
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.dep: %.cpp
#	$(CXX) -MM $< > $@
	@rm -f $@
	@$(CC) -MM $(CPPFLAGS) $< > $@.P
	@sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $*)\1.o $@ : ,g' < $@.P > $@
	@rm -f $@.P

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
	rm -rf $(BINDIR)

-include $(DEPS)

