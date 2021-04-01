CPPFLAGS=-DFREEGLUT_GLES -I/tmp/freeglut-native-gles/include
LDLIBS=-L/tmp/freeglut-native-gles/lib -lfreeglut-gles -lGLESv2 -lGLESv1_CM -lm
all: triangle
clean:
	rm -f *.o triangle
triangle: shader_utils.o
run:
	LD_LIBRARY_PATH=/tmp/freeglut-native-gles/lib/ ./triangle

.PHONY: all clean run
