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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int screen_width=800, screen_height=600;
GLuint vbo_sprite_vertices, vbo_sprite_texcoords;
GLuint program;
GLuint palette_id, texture_id, texture2_id;
GLint attribute_v_coord, attribute_v_texcoord;
GLint uniform_mvp, uniform_mypalette, uniform_mytexture, uniform_colorkey;
glm::mat4 mvp_background, mvp_player;

bool init_resources() {
	GLfloat sprite_vertices[] = {
	    0,    0, 0, 1,
	    1,    0, 0, 1,
	    0,    1, 0, 1,
	    1,    1, 0, 1,
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
	
	SDL_Surface* surface = IMG_Load("Ts01.bmp");
	if (surface == NULL) {
		cerr << "IMG_Load: " << SDL_GetError() << endl;
		return false;
	}

	glGenTextures(1, &palette_id);
	glBindTexture(GL_TEXTURE_2D, palette_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, // target
		0,	// level, 0 = base, no minimap,
		GL_RGBA, // internalformat
		256,	// width
		1,	// height
		0,	// border, always 0 in OpenGL ES
		GL_RGBA,	 // format
		GL_UNSIGNED_BYTE, // type
		surface->format->palette->colors);

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, // target
		0,	// level, 0 = base, no minimap,
		GL_LUMINANCE, // internalformat
		surface->w,	// width
		surface->h,	// height
		0,	// border, always 0 in OpenGL ES
		GL_LUMINANCE,	 // format
		GL_UNSIGNED_BYTE, // type
		surface->pixels);
	
	SDL_FreeSurface(surface);


	surface = IMG_Load("dinkm-01.bmp");
	if (surface == NULL) {
		cerr << "IMG_Load: " << SDL_GetError() << endl;
		return false;
	}
	glGenTextures(1, &texture2_id);
	glBindTexture(GL_TEXTURE_2D, texture2_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, // target
		0,	// level, 0 = base, no minimap,
		GL_LUMINANCE, // internalformat
		surface->w,	// width
		surface->h,	// height
		0,	// border, always 0 in OpenGL ES
		GL_LUMINANCE,	 // format
		GL_UNSIGNED_BYTE, // type
		surface->pixels);
	SDL_FreeSurface(surface);

	
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
	uniform_name = "mypalette";
	uniform_mypalette = glGetUniformLocation(program, uniform_name);
	if (uniform_mypalette == -1) {
		cerr << "Could not bind uniform " << uniform_name << endl;
		return false;
	}
	uniform_name = "mytexture";
	uniform_mytexture = glGetUniformLocation(program, uniform_name);
	if (uniform_mytexture == -1) {
		cerr << "Could not bind uniform " << uniform_name << endl;
		return false;
	}
	uniform_name = "colorkey";
	uniform_colorkey = glGetUniformLocation(program, uniform_name);
	if (uniform_colorkey == -1) {
		cerr << "Could not bind uniform " << uniform_name << endl;
		return false;
	}
	
	return true;
}

void logic() {
	float scale = sinf(SDL_GetTicks() / 1000.0) + 1;
	glm::mat4 projection = glm::ortho(0.0f, 1.0f*screen_width, 1.0f*screen_height, 0.0f);
	
	float move = screen_height/2;
	float angle = SDL_GetTicks() / 1000.0 * 45;  // 45° per second
	glm::vec3 axis_z(0, 0, 1);
	glm::mat4 m_transform;
	m_transform = glm::translate(glm::mat4(1), glm::vec3(0.375, 0.375, 0.))
		* glm::translate(glm::mat4(1.0f), glm::vec3(move, move, 0.0))
		* glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 0.0))
		* glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis_z)
		* glm::translate(glm::mat4(1.0f), glm::vec3(-400/2, -400/2, 0.0))
		* glm::scale(glm::mat4(1.0f), glm::vec3(400.0, 400.0, 0.0));
	mvp_background = projection * m_transform; // * view * model * anim;

	m_transform = glm::translate(glm::mat4(1), glm::vec3(0.375, 0.375, 0.))
		* glm::translate(glm::mat4(1.0f), glm::vec3(move, move, 0.0))
		* glm::rotate(glm::mat4(1.0f), -glm::radians(angle), axis_z)
		* glm::translate(glm::mat4(1.0f), glm::vec3(-100/2, -100/2, 0.0))
		* glm::scale(glm::mat4(1.0f), glm::vec3(100.0, 100.0, 0.0));
	mvp_player = projection * m_transform; // * view * model * anim;

	static Uint32 fps = 0;
	static Uint32 last_ticks = 0;
	fps++;
	if ((SDL_GetTicks() - last_ticks) > 1000) {
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,
					   "FPS: %d", fps);
		fps = 0;
		last_ticks = SDL_GetTicks();
	}

}

void render(SDL_Window* window) {
	glUseProgram(program);

	glClearColor(1.0, 0.1, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
    glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_mypalette, /*GL_TEXTURE*/0);
	glBindTexture(GL_TEXTURE_2D, palette_id);

    glActiveTexture(GL_TEXTURE1);
	glUniform1i(uniform_mytexture, /*GL_TEXTURE*/1);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glEnableVertexAttribArray(attribute_v_coord);
	// Describe our vertices array to OpenGL (it can't guess its format automatically)
	glBindBuffer(GL_ARRAY_BUFFER, vbo_sprite_vertices);
	glVertexAttribPointer(
		attribute_v_coord, // attribute
		4,                 // number of elements per vertex, here (x,y,z)
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
	glUniform1i(uniform_colorkey, -1);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp_background));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	

    glActiveTexture(GL_TEXTURE1);
	glUniform1i(uniform_mytexture, /*GL_TEXTURE*/1);
	glBindTexture(GL_TEXTURE_2D, texture2_id);
	glUniform1i(uniform_colorkey, 0);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp_player));
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
	glDeleteTextures(1, &palette_id);
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
	//SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
	if (SDL_GL_CreateContext(window) == NULL) {
		cerr << "Error: SDL_GL_CreateContext: " << SDL_GetError() << endl;
		return EXIT_FAILURE;
	}
	//SDL_GL_SetSwapInterval(0);
	
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
