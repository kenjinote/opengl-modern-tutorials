LDLIBS=-lm -lglut -lGLEW -lGL -lfreetype
CXXFLAGS=-I/usr/include/freetype2
all: text
clean:
	rm -f *.o text
text: ../common/shader_utils.o
.PHONY: all clean
