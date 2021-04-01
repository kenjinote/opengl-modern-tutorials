#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <SDL.h>

#include "../common/shader_utils.h"

GLuint program;
GLint attribute_coord1d;
GLint uniform_offset_x;
GLint uniform_scale_x;
GLuint texture_id;
GLint uniform_mytexture;

float offset_x = 0.0;
float scale_x = 1.0;

bool interpolate = false;
bool clamp = false;
bool showpoints = false;

GLuint vbo;

int init_resources() {
	program = create_program("graph.v.glsl", "graph.f.glsl");
	if (program == 0)
		return 0;

	attribute_coord1d = get_attrib(program, "coord1d");
	uniform_offset_x = get_uniform(program, "offset_x");
	uniform_scale_x = get_uniform(program, "scale_x");
	uniform_mytexture = get_uniform(program, "mytexture");

	if (attribute_coord1d == -1 || uniform_offset_x == -1 || uniform_scale_x == -1 || uniform_mytexture == -1)
		return 0;

	// Create our datapoints, store it as bytes
	GLbyte graph[2048];

	for (int i = 0; i < 2048; i++) {
		float x = (i - 1024.0) / 100.0;
		float y = sin(x * 10.0) / (1.0 + x * x);

		graph[i] = roundf(y * 128 + 128);
	}

	/* Upload the texture with our datapoints */
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 2048, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, graph);

	// Create the vertex buffer object
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Create an array with only x values.
	GLfloat line[101];

	// Fill it in just like an array
	for (int i = 0; i < 101; i++) {
		line[i] = (i - 50) / 50.0;
	}

	// Tell OpenGL to copy our array to the buffer object
	glBufferData(GL_ARRAY_BUFFER, sizeof line, line, GL_STATIC_DRAW);

	// Enable point size control in vertex shader
#ifndef GL_ES_VERSION_2_0
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

	return 1;
}

void free_resources() {
	glDeleteProgram(program);
}

void display() {
	glUseProgram(program);
	glUniform1i(uniform_mytexture, 0);

	glUniform1f(uniform_offset_x, offset_x);
	glUniform1f(uniform_scale_x, scale_x);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	/* Set texture wrapping mode */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);

	/* Set texture interpolation mode */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);

	/* Draw using the vertices in our vertex buffer object */
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glEnableVertexAttribArray(attribute_coord1d);
	glVertexAttribPointer(attribute_coord1d, 1, GL_FLOAT, GL_FALSE, 0, 0);

	/* Draw the line */
	glDrawArrays(GL_LINE_STRIP, 0, 101);

	/* Draw points as well, if requested */
	if (showpoints)
		glDrawArrays(GL_POINTS, 0, 101);
}

void keyDown(SDL_KeyboardEvent *ev) {
	switch (ev->keysym.scancode) {
	case SDL_SCANCODE_F1:
		interpolate = !interpolate;
		printf("Interpolation is now %s\n", interpolate ? "on" : "off");
		break;
	case SDL_SCANCODE_F2:
		clamp = !clamp;
		printf("Clamping is now %s\n", clamp ? "on" : "off");
		break;
	case SDL_SCANCODE_F3:
		showpoints = !showpoints;
		printf("Showing points is now %s\n", showpoints ? "on" : "off");
		break;
	case SDL_SCANCODE_LEFT:
		offset_x -= 0.1;
		break;
	case SDL_SCANCODE_RIGHT:
		offset_x += 0.1;
		break;
	case SDL_SCANCODE_UP:
		scale_x *= 1.5;
		break;
	case SDL_SCANCODE_DOWN:
		scale_x /= 1.5;
		break;
	case SDL_SCANCODE_HOME:
		offset_x = 0.0;
		scale_x = 1.0;
		break;
	default:
		break;
	}
}

void windowEvent(SDL_WindowEvent *ev) {
	switch(ev->event) {
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		glViewport(0, 0, ev->data1, ev->data2);
		break;
	default:
		break;
	}
}

void mainLoop(SDL_Window *window) {
	while (true) {
		display();
		SDL_GL_SwapWindow(window);

		bool redraw = false;

		while (!redraw) {
			SDL_Event ev;
			if (!SDL_WaitEvent(&ev))
				return;

			switch (ev.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				keyDown(&ev.key);
				redraw = true;
				break;
			case SDL_WINDOWEVENT:
				windowEvent(&ev.window);
				redraw = true;
				break;
			default:
				break;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *window = SDL_CreateWindow("My Graph",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			640, 480,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	GLenum glew_status = glewInit();

	if (GLEW_OK != glew_status) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
		return 1;
	}

	if (!GLEW_VERSION_2_0) {
		fprintf(stderr, "No support for OpenGL 2.0 found\n");
		return 1;
	}

	GLint max_units;

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &max_units);
	if (max_units < 1) {
		fprintf(stderr, "Your GPU does not have any vertex texture image units\n");
		return 1;
	}

	GLfloat range[2];

	glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);
	if (range[1] < 5.0)
		fprintf(stderr, "WARNING: point sprite range (%f, %f) too small\n", range[0], range[1]);

	printf("Use left/right to move horizontally.\n");
	printf("Use up/down to change the horizontal scale.\n");
	printf("Press home to reset the position and scale.\n");
	printf("Press F1 to toggle interpolation.\n");
	printf("Press F2 to toggle clamping.\n");
	printf("Press F3 to toggle drawing points.\n");

	if (!init_resources())
		return EXIT_FAILURE;

	mainLoop(window);

	free_resources();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
