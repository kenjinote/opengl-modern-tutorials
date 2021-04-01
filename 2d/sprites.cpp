/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/glut.h>
/* GLM */
// #define GLM_MESSAGES
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../common/shader_utils.h"

#include "res_texture.c"

int screen_width=800, screen_height=600;
GLuint vbo_sprite_vertices, vbo_sprite_texcoords;
GLuint program;
GLuint texture_id;
GLint attribute_v_coord, attribute_v_texcoord;
GLint uniform_mvp, uniform_mytexture;

int init_resources()
{
  GLfloat sprite_vertices[] = {
      0,    0, 0, 1,
    256,    0, 0, 1,
      0,  256, 0, 1,
    256,  256, 0, 1,
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

  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, // target
	       0,  // level, 0 = base, no minimap,
	       GL_RGB, // internalformat
	       res_texture.width,  // width
	       res_texture.height,  // height
	       0,  // border, always 0 in OpenGL ES
	       GL_RGB,  // format
	       GL_UNSIGNED_BYTE, // type
	       res_texture.pixel_data);

  GLint link_ok = GL_FALSE;

  GLuint vs, fs;
  if ((vs = create_shader("sprites.v.glsl", GL_VERTEX_SHADER))   == 0) return 0;
  if ((fs = create_shader("sprites.f.glsl", GL_FRAGMENT_SHADER)) == 0) return 0;

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
  if (!link_ok) {
    fprintf(stderr, "glLinkProgram:");
    print_log(program);
    return 0;
  }

  const char* attribute_name;
  attribute_name = "v_coord";
  attribute_v_coord = glGetAttribLocation(program, attribute_name);
  if (attribute_v_coord == -1) {
    fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
    return 0;
  }
  attribute_name = "v_texcoord";
  attribute_v_texcoord = glGetAttribLocation(program, attribute_name);
  if (attribute_v_texcoord == -1) {
    fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
    return 0;
  }
  const char* uniform_name;
  uniform_name = "mvp";
  uniform_mvp = glGetUniformLocation(program, uniform_name);
  if (uniform_mvp == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }
  uniform_name = "mytexture";
  uniform_mytexture = glGetUniformLocation(program, uniform_name);
  if (uniform_mytexture == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }

  return 1;
}

void onDisplay()
{
  glUseProgram(program);
  glUniform1i(uniform_mytexture, /*GL_TEXTURE*/0);

  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

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
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(attribute_v_coord);
  glDisableVertexAttribArray(attribute_v_texcoord);
  glutSwapBuffers();
}

void onIdle() {
  float scale = glutGet(GLUT_ELAPSED_TIME) / 1000.0 * .2;  // 20% per second
  glm::mat4 projection = glm::ortho(0.0f, 1.0f*screen_width*scale, 1.0f*screen_height*scale, 0.0f);

  float move = 128;
  float angle = glutGet(GLUT_ELAPSED_TIME) / 1000.0 * 45;  // 45° per second
  glm::vec3 axis_z(0, 0, 1);
  glm::mat4 m_transform = glm::translate(glm::mat4(1.0f), glm::vec3(move, move, 0.0))
    * glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis_z)
    * glm::translate(glm::mat4(1.0f), glm::vec3(-256/2, -256/2, 0.0));

  glm::mat4 mvp = projection * m_transform; // * view * model * anim;
  glUseProgram(program);
  glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
  glutPostRedisplay();
}

void onReshape(int width, int height) {
  screen_width = width;
  screen_height = height;
  glViewport(0, 0, screen_width, screen_height);
}

void free_resources()
{
  glDeleteProgram(program);
  glDeleteBuffers(1, &vbo_sprite_vertices);
  glDeleteBuffers(1, &vbo_sprite_texcoords);
  glDeleteTextures(1, &texture_id);
}


int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE);
  glutInitWindowSize(screen_width, screen_height);
  glutCreateWindow("My Sprites");

  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
    return 1;
  }

  if (!GLEW_VERSION_2_0) {
    fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
    return 1;
  }

  if (init_resources()) {
    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutIdleFunc(onIdle);
    glEnable(GL_BLEND);
    //glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutMainLoop();
  }

  free_resources();
  return 0;
}
