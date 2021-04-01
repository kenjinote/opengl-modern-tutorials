/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler, Guus Sliepen
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <GL/glew.h>
#include <SDL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/closest_point.hpp>
#include "../common/shader_utils.h"

#include "res_texture.c"

int window_width=800, window_height=600;
GLuint vbo_cube_vertices, vbo_cube_texcoords;
GLuint ibo_cube_elements;
GLuint program;
GLuint texture_id;
GLint attribute_coord3d, attribute_texcoord;
GLint uniform_mvp, uniform_mytexture, uniform_color;

#define NCUBES 10
glm::vec3 positions[3 * NCUBES];
glm::vec3 rotspeeds[3 * NCUBES];
bool highlight[NCUBES];

float camera_angle = 0;
glm::vec3 camera_position(0.0, 2.0, 4.0);

int init_resources() {
	static const GLfloat cube_vertices[] = {
		// front
		-1.0, -1.0, +1.0,
		+1.0, -1.0, +1.0,
		+1.0, +1.0, +1.0,
		-1.0, +1.0, +1.0,
		// top
		-1.0, +1.0, +1.0,
		+1.0, +1.0, +1.0,
		+1.0, +1.0, -1.0,
		-1.0, +1.0, -1.0,
		// back
		+1.0, -1.0, -1.0,
		-1.0, -1.0, -1.0,
		-1.0, +1.0, -1.0,
		+1.0, +1.0, -1.0,
		// bottom
		-1.0, -1.0, -1.0,
		+1.0, -1.0, -1.0,
		+1.0, -1.0, +1.0,
		-1.0, -1.0, +1.0,
		// left
		-1.0, -1.0, -1.0,
		-1.0, -1.0, +1.0,
		-1.0, +1.0, +1.0,
		-1.0, +1.0, -1.0,
		// right
		+1.0, -1.0, +1.0,
		+1.0, -1.0, -1.0,
		+1.0, +1.0, -1.0,
		+1.0, +1.0, +1.0,
	};

	glGenBuffers(1, &vbo_cube_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof cube_vertices, cube_vertices, GL_STATIC_DRAW);

	static const GLfloat cube_texcoords[2*4*6] = {
		0.0, 1.0,  1.0, 1.0,  1.0, 0.0,  0.0, 0.0,
		0.0, 1.0,  1.0, 1.0,  1.0, 0.0,  0.0, 0.0,
		0.0, 1.0,  1.0, 1.0,  1.0, 0.0,  0.0, 0.0,
		0.0, 1.0,  1.0, 1.0,  1.0, 0.0,  0.0, 0.0,
		0.0, 1.0,  1.0, 1.0,  1.0, 0.0,  0.0, 0.0,
		0.0, 1.0,  1.0, 1.0,  1.0, 0.0,  0.0, 0.0,
	};

	glGenBuffers(1, &vbo_cube_texcoords);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_texcoords);
	glBufferData(GL_ARRAY_BUFFER, sizeof cube_texcoords, cube_texcoords, GL_STATIC_DRAW);

	static const GLushort cube_elements[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// top
		4, 5, 6,
		6, 7, 4,
		// back
		8, 9, 10,
		10, 11, 8,
		// bottom
		12, 13, 14,
		14, 15, 12,
		// left
		16, 17, 18,
		18, 19, 16,
		// right
		20, 21, 22,
		22, 23, 20,
	};

	glGenBuffers(1, &ibo_cube_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof cube_elements, cube_elements, GL_STATIC_DRAW);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, res_texture.width, res_texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, res_texture.pixel_data);

	program = create_program("cube.v.glsl", "cube.f.glsl");
	if(program == 0)
		return 0;

	attribute_coord3d = get_attrib(program, "coord3d");
	attribute_texcoord = get_attrib(program, "texcoord");
	uniform_mvp = get_uniform(program, "mvp");
	uniform_mytexture = get_uniform(program, "mytexture");
	uniform_color = get_uniform(program, "color");

	if(attribute_coord3d == -1 || attribute_texcoord == -1 || uniform_mvp == -1 || uniform_mytexture == -1 || uniform_color == -1)
		return 0;

	srand48(time(NULL));

	for(int i = 0; i < NCUBES; i++) {
		positions[i] = glm::vec3((drand48() - 0.5) * 2, (drand48() - 0.5) * 2, (drand48() - 0.5) * 2);
		rotspeeds[i] = glm::vec3(drand48() * 5, drand48() * 5, drand48() * 5);
	}

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return 1;
}

void free_resources() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo_cube_vertices);
	glDeleteBuffers(1, &vbo_cube_texcoords);
	glDeleteBuffers(1, &ibo_cube_elements);
	glDeleteTextures(1, &texture_id);
}

void onIdle() {
}

void display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	glUseProgram(program);
	glUniform1i(uniform_mytexture, /*GL_TEXTURE*/0);
	glEnableVertexAttribArray(attribute_coord3d);
	// Describe our vertices array to OpenGL (it can't guess its format automatically)
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
	glVertexAttribPointer(attribute_coord3d, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(attribute_texcoord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_texcoords);
	glVertexAttribPointer(attribute_texcoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_elements);
	int size;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glm::mat4 view = glm::lookAt(camera_position, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 projection = glm::perspective(45.0f, 1.0f*window_width/window_height, 0.1f, 10.0f);

	static const GLfloat color_normal[4] = {1, 1, 1, 1};
	static const GLfloat color_highlight[4] = {2, 2, 2, 1};

	float angle = SDL_GetTicks() / 1000.0 * glm::radians(15.0);	// base 15° per second

	for(int i = 0; i < NCUBES; i++) {
		glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), positions[i]), glm::vec3(0.2, 0.2, 0.2));

		glm::mat4 anim = 
			glm::rotate(glm::mat4(1.0f), angle * rotspeeds[i].x, glm::vec3(1, 0, 0)) *	// X axis
			glm::rotate(glm::mat4(1.0f), angle * rotspeeds[i].y, glm::vec3(0, 1, 0)) *	// Y axis
			glm::rotate(glm::mat4(1.0f), angle * rotspeeds[i].z, glm::vec3(0, 0, 1));	// Z axis

		glm::mat4 mvp = projection * view * model * anim;
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

		if(highlight[i])
			glUniform4fv(uniform_color, 1, color_highlight);
		else
			glUniform4fv(uniform_color, 1, color_normal);

		glStencilFunc(GL_ALWAYS, i + 1, -1);

		/* Push each element in buffer_vertices to the vertex shader */
		glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
	}

	glDisableVertexAttribArray(attribute_coord3d);
	glDisableVertexAttribArray(attribute_texcoord);
}

void onReshape(int width, int height) {
	window_width = width;
	window_height = height;
	glViewport(0, 0, window_width, window_height);
}

void keyDown(SDL_KeyboardEvent *ev) {
	switch(ev->keysym.scancode) {
		case SDL_SCANCODE_LEFT:
			camera_angle -= glm::radians(5.0);
			break;
		case SDL_SCANCODE_RIGHT:
			camera_angle += glm::radians(5.0);
			break;
		default:
			break;
	}

	camera_position = glm::rotateY(glm::vec3(0.0, 2.0, 4.0), camera_angle);
}

void mouseButtonDown(SDL_MouseButtonEvent *ev) {
	/* Read color, depth and stencil index from the framebuffer */
	GLbyte color[4];
	GLfloat depth;
	GLuint index;
	
	glReadPixels(ev->x, window_height - ev->y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);
	glReadPixels(ev->x, window_height - ev->y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	glReadPixels(ev->x, window_height - ev->y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

	printf("Clicked on pixel %d, %d, color %02hhx%02hhx%02hhx%02hhx, depth %f, stencil index %u\n",
			ev->x, ev->y, color[0], color[1], color[2], color[3], depth, index);

	/* Convert from window coordinates to object coordinates */
	glm::mat4 view = glm::lookAt(camera_position, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 projection = glm::perspective(45.0f, 1.0f*window_width/window_height, 0.1f, 10.0f);
	glm::vec4 viewport = glm::vec4(0, 0, window_width, window_height);

	glm::vec3 wincoord = glm::vec3(ev->x, window_height - ev->y - 1, depth);
	glm::vec3 objcoord = glm::unProject(wincoord, view, projection, viewport);

	/* Which box is nearest to those object coordinates? */
	int closest_i = 0;

	for(int i = 1; i < NCUBES; i++) {
		if(glm::distance(objcoord, positions[i]) < glm::distance(objcoord, positions[closest_i]))
			closest_i = i;
	}

	printf("Coordinates in object space: %f, %f, %f, closest to center of box %d\n",
			objcoord.x, objcoord.y, objcoord.z, closest_i + 1);

	/* Ray casting */
	closest_i = -1;
	float closest_distance = 1.0 / 0.0; // infinity

	wincoord.z = 0.99;
	glm::vec3 point1 = camera_position;
	glm::vec3 point2 = glm::unProject(wincoord, view, projection, viewport);

	for(int i = 0; i < NCUBES; i++) {
		glm::vec3 cpol = glm::closestPointOnLine(positions[i], point1, point2);
		float dtol = glm::distance(positions[i], cpol);
		float distance = glm::distance(positions[i], camera_position);
		if(dtol < 0.2 * sqrtf(3.0) && distance < closest_distance) {
			closest_i = i;
			closest_distance = distance;
		}
	}

	printf("Closest along camera ray: %d\n", closest_i + 1);

	/* Toggle highlighting of the selected object */
	if(index)
		highlight[index - 1] = !highlight[index - 1];
}

void windowEvent(SDL_WindowEvent *ev) {
	switch(ev->event) {
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		window_width = ev->data1;
		window_height = ev->data2;
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

		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				keyDown(&ev.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
				mouseButtonDown(&ev.button);
				break;
			case SDL_WINDOWEVENT:
				windowEvent(&ev.window);
				break;
			default:
				break;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window *window = SDL_CreateWindow("Object selection",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			window_width, window_height,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
		return 1;
	}

	if (!GLEW_VERSION_2_0) {
		fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
		return 1;
	}

	printf("Click on a box to highlight it.\n");
	printf("Use left/right to rotate the scene.\n");

	if (!init_resources())
		return EXIT_FAILURE;

	mainLoop(window);

	free_resources();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
