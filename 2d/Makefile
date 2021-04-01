CC=g++
LDLIBS=-lglut -lGLEW -lGL -lm
all: sprites
clean:
	rm -f *.o sprites
sprites.o: res_texture.c
sprites: ../common/shader_utils.o
.PHONY: all clean
