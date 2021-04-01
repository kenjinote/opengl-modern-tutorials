CC=g++
LDLIBS=-lglut -lGLEW -lGL -lm
all: cube
clean:
	rm -f *.o cube
cube: ../common/shader_utils.o
cube.o: res_texture.c
.PHONY: all clean
