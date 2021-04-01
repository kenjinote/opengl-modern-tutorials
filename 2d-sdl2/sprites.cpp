/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */
#include <cstdlib>
#include <iostream>
using namespace std;

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using SDL2 for the base window and OpenGL context init */
#include "SDL.h"
/* Using SDL2_image to load PNG & JPG in memory */
#include "SDL_image.h"

#include "../common-sdl2/shader_utils.h"

/* GLM */
// #define GLM_MESSAGES
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int screen_width=800, screen_height=600;
GLuint vbo_sprite_vertices, vbo_sprite_texcoords;
GLuint program;
GLuint texture_id;
GLint attribute_v_coord, attribute_v_texcoord;
GLint uniform_mvp, uniform_mytexture;

static unsigned int fps_start = 0;
static unsigned int fps_frames = 0;

bool init_resources() {
	GLfloat sprite_vertices[] = {
	    0,    0, 1,
	  256,    0, 1,
	    0,  256, 1,
	  256,  256, 1,
	};
	glGenBuffers(1, &vbo_sprite_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_sprite_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sprite_vertices), sprite_vertices, GL_STATIC_DRAW);
	
	GLfloat sprite_texcoords[] = {
		0.0, 0.0,
		1.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
	};
	glGenBuffers(1, &vbo_sprite_texcoords);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_sprite_texcoords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sprite_texcoords), sprite_texcoords, GL_STATIC_DRAW);
	
	SDL_Surface* res_texture = IMG_Load("res_texture.png");
	if (res_texture == NULL) {
		cerr << "IMG_Load: " << SDL_GetError() << endl;
		return false;
	}
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, // target
		0,	// level, 0 = base, no minimap,
		GL_RGBA, // internalformat
		res_texture->w,	// width
		res_texture->h,	// height
		0,	// border, always 0 in OpenGL ES
		GL_RGBA,	 // format
		GL_UNSIGNED_BYTE, // type
		res_texture->pixels);
	SDL_FreeSurface(res_texture);
	
	GLint link_ok = GL_FALSE;
	
	GLuint vs, fs;
	if ((vs = create_shader("sprites.v.glsl", GL_VERTEX_SHADER))   == 0) return false;
	if ((fs = create_shader("sprites.f.glsl", GL_FRAGMENT_SHADER)) == 0) return false;

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		cerr << "glLinkProgram:";
		print_log(program);
		return false;
	}

	const char* attribute_name;
	attribute_name = "v_coord";
	attribute_v_coord = glGetAttribLocation(program, attribute_name);
	if (attribute_v_coord == -1) {
		cerr << "Could not bind attribute " << attribute_name << endl;
		return false;
	}
	attribute_name = "v_texcoord";
	attribute_v_texcoord = glGetAttribLocation(program, attribute_name);
	if (attribute_v_texcoord == -1) {
		cerr << "Could not bind attribute " << attribute_name << endl;
		return false;
	}
	const char* uniform_name;
	uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if (uniform_mvp == -1) {
		cerr << "Could not bind uniform " << uniform_name << endl;
		return false;
	}
	uniform_name = "mytexture";
	uniform_mytexture = glGetUniformLocation(program, uniform_name);
	if (uniform_mytexture == -1) {
		cerr << "Could not bind uniform " << uniform_name << endl;
		return false;
	}
	
	fps_start = SDL_GetTicks();

	return true;
}

void logic() {
	{
		/* FPS count */
		fps_frames++;
		int delta_t = SDL_GetTicks() - fps_start;
		if (delta_t > 1000) {
			cout << 1000.0 * fps_frames / delta_t << endl;
			fps_frames = 0;
			fps_start = SDL_GetTicks();
		}
	}

	float scale = SDL_GetTicks() / 1000.0 * .2;  // 20% per second
	glm::mat4 projection = glm::ortho(0.0f, 1.0f*screen_width*scale, 1.0f*screen_height*scale, 0.0f);
	
	float move = 128;
	float angle = SDL_GetTicks() / 1000.0 * 45;  // 45° per second
	glm::vec3 axis_z(0, 0, 1);
	glm::mat4 m_transform = glm::translate(glm::mat4(1.0f), glm::vec3(move, move, 0.0))
		* glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis_z)
		* glm::translate(glm::mat4(1.0f), glm::vec3(-256/2, -256/2, 0.0));
	
	glm::mat4 mvp = projection * m_transform; // * view * model * anim;
	glm::mat3 mvp2D(mvp[0].xyw(), mvp[1].xyw(), mvp[3].xyw());
	glUseProgram(program);
	glUniformMatrix3fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp2D));
}

void render(SDL_Window* window) {
	glUseProgram(program);
	
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_mytexture, /*GL_TEXTURE*/0);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glEnableVertexAttribArray(attribute_v_coord);
	// Describe our vertices array to OpenGL (it can't guess its format automatically)
	glBindBuffer(GL_ARRAY_BUFFER, vbo_sprite_vertices);
	glVertexAttribPointer(
		attribute_v_coord, // attribute
		3,                 // number of elements per vertex, here (x,y,w)
		GL_FLOAT,          // the type of each element
		GL_FALSE,          // take our values as-is
		0,                 // no extra data between each position
		0                  // offset of first element
	);
	
	glEnableVertexAttribArray(attribute_v_texcoord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_sprite_texcoords);
	glVertexAttribPointer(
		attribute_v_texcoord, // attribute
		2,                  // number of elements per vertex, here (x,y)
		GL_FLOAT,           // the type of each element
		GL_FALSE,           // take our values as-is
		0,                  // no extra data between each position
		0                   // offset of first element
	);
	
	/* Push each element in buffer_vertices to the vertex shader */
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glDisableVertexAttribArray(attribute_v_coord);
	glDisableVertexAttribArray(attribute_v_texcoord);
	SDL_GL_SwapWindow(window);
}

void onResize(int width, int height) {
	screen_width = width;
	screen_height = height;
	glViewport(0, 0, screen_width, screen_height);
}

void free_resources() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo_sprite_vertices);
	glDeleteBuffers(1, &vbo_sprite_texcoords);
	glDeleteTextures(1, &texture_id);
}

void mainLoop(SDL_Window* window) {
	while (true) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				return;
			if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				onResize(ev.window.data1, ev.window.data2);
		}
		logic();
		render(window);
	}
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("My Sprites",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		screen_width, screen_height,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	if (window == NULL) {
		cerr << "Error: can't create window: " << SDL_GetError() << endl;
		return EXIT_FAILURE;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	if (SDL_GL_CreateContext(window) == NULL) {
		cerr << "Error: SDL_GL_CreateContext: " << SDL_GetError() << endl;
		return EXIT_FAILURE;
	}
	// Don't limit FPS (no VSync)
	SDL_GL_SetSwapInterval(0);

	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK) {
		cerr << "Error: glewInit: " << glewGetErrorString(glew_status) << endl;
		return EXIT_FAILURE;
	}
	if (!GLEW_VERSION_2_0) {
		cerr << "Error: your graphic card does not support OpenGL 2.0" << endl;
		return EXIT_FAILURE;
	}

	if (!init_resources())
		return EXIT_FAILURE;

    glEnable(GL_BLEND);
    //glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
    mainLoop(window);
	
	free_resources();
	return EXIT_SUCCESS;
}
