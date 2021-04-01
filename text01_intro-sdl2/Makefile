CPPFLAGS=$(shell sdl2-config --cflags) $(shell $(PKG_CONFIG) freetype2 --cflags) $(EXTRA_CPPFLAGS)
LDLIBS=$(shell sdl2-config --libs) $(shell $(PKG_CONFIG) freetype2 --libs) -lGLEW $(EXTRA_LDLIBS)
EXTRA_LDLIBS?=-lGL
PKG_CONFIG?=pkg-config
all: text
clean:
	rm -f *.o text
text: ../common-sdl2/shader_utils.o
.PHONY: all clean
