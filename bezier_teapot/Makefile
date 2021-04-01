CXXFLAGS=-ggdb
LDLIBS=-lglut -lGLEW -lGL -lm -lstdc++
all: teapot
clean:
	rm -f *.o teapot
teapot: ../common/shader_utils.o teapot.o
.PHONY: all clean
