/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler, Guus Sliepen
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/freeglut.h>
/* GLM */
// #define GLM_MESSAGES
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL/SOIL.h>
#include "../common/shader_utils.h"

#include <iostream>
using namespace std;

int screen_width=800, screen_height=600;
GLuint program;
GLuint mytexture_id, mytexture_sunlit_id;
GLuint sphere_vbo = -1;
GLint attribute_v_coord = -1, attribute_v_normal = 1;
GLint uniform_m = -1, uniform_v = -1, uniform_p = -1,
    uniform_m_3x3_inv_transp = -1, uniform_v_inv = -1,
    uniform_mytexture = -1, uniform_mytexture_sunlit = -1, uniform_mytexture_ST = -1;

struct demo {
    const char* texture_filename;
    const char* vshader_filename;
    const char* fshader_filename;
};
struct demo demos[] = {
    // Textures Spheres
    { "Earthmap720x360_grid.jpg", "sphere.v.glsl", "sphere.f.glsl" },
    { "Earthmap720x360_grid.jpg", "sphere.v.glsl", "sphere_ST.f.glsl" },
    // Lighting Textured Surfaces
      { "Earthmap720x360_grid.jpg", "sphere-gouraud.v.glsl", "sphere-gouraud.f.glsl" },
    // Glossy Textures
    { "Land_shallow_topo_alpha_2048.png", "sphere-gouraud.v.glsl", "sphere-gouraud-glossy.f.glsl" },
    { "Land_shallow_topo_alpha_2048.png", "sphere-phong.v.glsl", "sphere-phong.f.glsl" },
    // Transparent Textures
    { "Land_shallow_topo_alpha_2048.png", "sphere.v.glsl", "sphere_discard.f.glsl" },
    { "Land_shallow_topo_alpha_2048.png", "sphere.v.glsl", "sphere.f.glsl" },
    { "Land_shallow_topo_alpha_2048.png", "sphere.v.glsl", "sphere_oceans.f.glsl" },
    // Layers of Textures
    { "Earth_lights_lrg.jpg", "sphere-sunlit.v.glsl", "sphere-sunlit.f.glsl" },
};
int cur_demo = 0;

GLuint sphere(float radius, int slices, int stacks) {
  GLuint vbo;
  int n = 2 * (slices + 1) * stacks;
  int i = 0;
  glm::vec3 points[n];
  
  for (float theta = -M_PI / 2; theta < M_PI / 2 - 0.0001; theta += M_PI / stacks) {
    for (float phi = -M_PI; phi <= M_PI + 0.0001; phi += 2 * M_PI / slices) {
      points[i++] = glm::vec3(cos(theta) * sin(phi), -sin(theta), cos(theta) * cos(phi));
      points[i++] = glm::vec3(cos(theta + M_PI / stacks) * sin(phi), -sin(theta + M_PI / stacks), cos(theta + M_PI / stacks) * cos(phi));
    }
  }
  
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof points, points, GL_STATIC_DRAW);
  
  return vbo;
}

int init_resources()
{
  printf("init_resources: %s %s %s\n",
         demos[cur_demo].texture_filename, demos[cur_demo].vshader_filename, demos[cur_demo].fshader_filename);
  // TODO: try to optimize on Android by using ETC1 texture format
  mytexture_id = SOIL_load_OGL_texture
    (
     demos[cur_demo].texture_filename,
     SOIL_LOAD_AUTO,
     SOIL_CREATE_NEW_ID,
     SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
     );
  if (mytexture_id == 0)
    cerr << "SOIL loading error: '" << SOIL_last_result() << "' (" << demos[cur_demo].texture_filename << ")" << endl;

  // Day-time texture:
  mytexture_sunlit_id = SOIL_load_OGL_texture
    (
     "Land_shallow_topo_2048.jpg",
     SOIL_LOAD_AUTO,
     SOIL_CREATE_NEW_ID,
     SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
     );
  if (mytexture_sunlit_id == 0)
    cerr << "SOIL loading error: '" << SOIL_last_result() << "' (" << "Land_shallow_topo_2048.jpg" << ")" << endl;

  GLint link_ok = GL_FALSE;

  GLuint vs, fs;
  if ((vs = create_shader(demos[cur_demo].vshader_filename, GL_VERTEX_SHADER))   == 0) return 0;
  if ((fs = create_shader(demos[cur_demo].fshader_filename, GL_FRAGMENT_SHADER)) == 0) return 0;

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
  attribute_name = "v_normal";
  attribute_v_normal = glGetAttribLocation(program, attribute_name);
  if (attribute_v_normal == -1) {
    fprintf(stderr, "Warning: Could not bind attribute %s\n", attribute_name);
  }
  const char* uniform_name;
  uniform_name = "m";
  uniform_m = glGetUniformLocation(program, uniform_name);
  if (uniform_m == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }
  uniform_name = "v";
  uniform_v = glGetUniformLocation(program, uniform_name);
  if (uniform_v == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }
  uniform_name = "p";
  uniform_p = glGetUniformLocation(program, uniform_name);
  if (uniform_p == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }
  uniform_name = "m_3x3_inv_transp";
  uniform_m_3x3_inv_transp = glGetUniformLocation(program, uniform_name);
  if (uniform_m_3x3_inv_transp == -1) {
    fprintf(stderr, "Warning: Could not bind uniform %s\n", uniform_name);
  }
  uniform_name = "v_inv";
  uniform_v_inv = glGetUniformLocation(program, uniform_name);
  if (uniform_v_inv == -1) {
    fprintf(stderr, "Warning: Could not bind uniform %s\n", uniform_name);
  }
  uniform_name = "mytexture";
  uniform_mytexture = glGetUniformLocation(program, uniform_name);
  if (uniform_mytexture == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;
  }
  uniform_name = "mytexture_sunlit";
  uniform_mytexture_sunlit = glGetUniformLocation(program, uniform_name);
  if (uniform_mytexture_sunlit == -1) {
    fprintf(stderr, "Warning: Could not bind uniform %s\n", uniform_name);
  }
  if (strstr(demos[cur_demo].fshader_filename, "_ST")) {
      uniform_name = "mytexture_ST";
      uniform_mytexture_ST = glGetUniformLocation(program, uniform_name);
      if (uniform_mytexture_ST == -1) {
          fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
          return 0;
      }
  }

  sphere_vbo = sphere(1, 30, 30);

  return 1;
}

void free_resources()
{
  glDeleteProgram(program);
  glDeleteTextures(1, &mytexture_id);
  glDeleteTextures(1, &mytexture_sunlit_id);
}

void logic() {
  float angle = glutGet(GLUT_ELAPSED_TIME) / 1000.0 * 30;  // 30° per second
  glm::mat4 anim = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));

  // Fix for Blender- or GLUT-style orientation (Z-is-up).
  // Not necessary since switching to our own sphere code, but require
  // fixing 'latitudeLongitude' in the shaders.
  glm::mat4 fix_orientation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), glm::vec3(1, 0, 0));

  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, -2.0));
  glm::mat4 view = glm::lookAt(glm::vec3(0.0, 2.0, 0.0), glm::vec3(0.0, 0.0, -2.0), glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f*screen_width/screen_height, 0.1f, 10.0f);

  //glm::mat4 mvp = projection * view * model * anim * fix_orientation;
  //glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));


  glUseProgram(program);
  glm::mat4 m = model * anim * fix_orientation;
  glUniformMatrix4fv(uniform_m, 1, GL_FALSE, glm::value_ptr(m));
  /* Transform normal vectors with transpose of inverse of upper left
     3x3 model matrix (ex-gl_NormalMatrix): */
  glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(glm::mat3(m)));
  glUniformMatrix3fv(uniform_m_3x3_inv_transp, 1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));

  glUniformMatrix4fv(uniform_v, 1, GL_FALSE, glm::value_ptr(view));
  glm::mat4 v_inv = glm::inverse(view);
  glUniformMatrix4fv(uniform_v_inv, 1, GL_FALSE, glm::value_ptr(v_inv));

  glUniformMatrix4fv(uniform_p, 1, GL_FALSE, glm::value_ptr(projection));

  // tiling and offset
  if (uniform_mytexture_ST >= 0)
      glUniform4f(uniform_mytexture_ST, 2,1, 0,-.05);

  glutPostRedisplay();
}

void draw()
{
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(program);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(uniform_mytexture, /*GL_TEXTURE*/0);
  glBindTexture(GL_TEXTURE_2D, mytexture_id);

  glActiveTexture(GL_TEXTURE1);
  glUniform1i(uniform_mytexture_sunlit, /*GL_TEXTURE*/1);
  glBindTexture(GL_TEXTURE_2D, mytexture_sunlit_id);

  // To be used when FreeGLUT 3.0.0 is out :)
  // glutSetVertexAttribCoord3(attribute_v_coord);
  // glutSetVertexAttribNormal(attribute_v_normal);
  // glutSolidSphere(1.0,30,30);

  glEnableVertexAttribArray(attribute_v_coord);
  glEnableVertexAttribArray(attribute_v_normal);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
  glVertexAttribPointer(attribute_v_coord, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(attribute_v_normal, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glCullFace(GL_FRONT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * 31 * 30);

  glCullFace(GL_BACK);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * 31 * 30);
}

void onDisplay() {
  logic();
  draw();
  glutSwapBuffers();
}

void onReshape(int width, int height) {
  screen_width = width;
  screen_height = height;
  glViewport(0, 0, screen_width, screen_height);
}

void onMouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      free_resources();
      cur_demo = (cur_demo + 1) % (sizeof(demos)/sizeof(struct demo));
      init_resources();
  }
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitContextVersion(2,0);
  glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE|GLUT_DEPTH);
  glutInitWindowSize(screen_width, screen_height);
  glutCreateWindow("Textured Spheres");

  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
    return EXIT_FAILURE;
  }

  if (!GLEW_VERSION_2_0) {
    fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
    return EXIT_FAILURE;
  }

  if (init_resources()) {
    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutMouseFunc(onMouse);

    // glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);

    // glAlphaFunc(GL_GREATER, 0.1);
    // glEnable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glutMainLoop();
  }

  free_resources();
  return EXIT_SUCCESS;
}
