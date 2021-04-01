CPPFLAGS=$(shell sdl2-config --cflags) $(EXTRA_CPPFLAGS)
LDLIBS=$(shell sdl2-config --libs) $(EXTRA_LDLIBS)
EXTRA_LDLIBS?=-lGL -lm
EXTRA_CPPFLAGS?=-Ofast -Wall
all: glescraft
glescraft: glescraft.o shader_utils.o
	$(CXX) -o $@ $^ $(LDLIBS)
clean:
	rm -f *.o glescraft
.PHONY: all clean
