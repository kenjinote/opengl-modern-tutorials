CPPFLAGS=$(shell pkg-config sdl2 glew --cflags) $(EXTRA_CPPFLAGS)
LDLIBS=$(shell pkg-config sdl2 glew --libs) $(EXTRA_LDLIBS)
EXTRA_LDLIBS?=-lGL -lm
EXTRA_CPPFLAGS?=-Wall -g
all: select
select: select.o ../common-sdl2/shader_utils.o
	$(CXX) -o $@ $^ $(LDLIBS)
clean:
	rm -f *.o select
.PHONY: all clean
