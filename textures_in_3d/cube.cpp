/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
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
GLuint normalmap_id;
GLuint sphere_vbo = -1;
GLint attribute_v_coord = -1, attribute_v_normal = -1, attribute_v_texcoords = -1, attribute_v_tangent = 1;
GLint uniform_m = -1, uniform_v = -1, uniform_p = -1,
    uniform_m_3x3_inv_transp = -1, uniform_v_inv = -1,
    uniform_normalmap = -1;

struct demo {
    const char* texture_filename;
    const char* vshader_filename;
    const char* fshader_filename;
};
struct demo demos[] = {
    // Lighting of Bumpy Surfaces
    { "IntP_Brick_NormalMap.png", "cube.v.glsl", "cube.f.glsl" },
};
int cur_demo = 0;



class Mesh {
private:
  GLuint vbo_vertices, vbo_normals, vbo_texcoords, vbo_tangents, ibo_elements;
public:
  vector<glm::vec4> vertices;
  vector<glm::vec3> normals;
  vector<glm::vec2> texcoords;
  vector<glm::vec3> tangents;
  vector<GLushort> elements;
  glm::mat4 object2world;

  Mesh() : vbo_vertices(0), vbo_normals(0), vbo_texcoords(0), vbo_tangents(0),
           ibo_elements(0), object2world(glm::mat4(1)) {}
  ~Mesh() {
    if (vbo_vertices != 0)
      glDeleteBuffers(1, &vbo_vertices);
    if (vbo_normals != 0)
      glDeleteBuffers(1, &vbo_normals);
    if (vbo_texcoords != 0)
      glDeleteBuffers(1, &vbo_texcoords);
    if (vbo_tangents != 0)
      glDeleteBuffers(1, &vbo_tangents);
    if (ibo_elements != 0)
      glDeleteBuffers(1, &ibo_elements);
  }

  /**
   * Express surface tangent in object space
   * http://www.terathon.com/code/tangent.html
   * http://www.3dkingdoms.com/weekly/weekly.php?a=37
   */
  void compute_tangents() {
    tangents.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));

    for (int i = 0; i < elements.size(); i+=3) {
      int i1 = elements.at(i);
      int i2 = elements.at(i+1);
      int i3 = elements.at(i+2);
      glm::vec3 p1(vertices.at(i1));
      glm::vec3 p2(vertices.at(i2));
      glm::vec3 p3(vertices.at(i3));
      glm::vec2 uv1 = texcoords.at(i1);
      glm::vec2 uv2 = texcoords.at(i2);
      glm::vec2 uv3 = texcoords.at(i3);

      glm::vec3 p1p2 = p2 - p1;
      glm::vec3 p1p3 = p3 - p1;
      glm::vec2 uv1uv2 = uv2 - uv1;
      glm::vec2 uv1uv3 = uv3 - uv1;

      float c = uv1uv2.s * uv1uv3.t - uv1uv3.s * uv1uv2.t;
      if (c != 0) {
        float mul = 1.0 / c;
        glm::vec3 tangent = (p1p2 * uv1uv3.t - p1p3 * uv1uv2.t) * mul;
        tangents.at(i1) = tangents.at(i2) = tangents.at(i3) = glm::normalize(tangent);
      }
    }
  }

  /**
   * Store object vertices, normals and/or elements in graphic card
   * buffers
   */
  void upload() {
    if (this->vertices.size() > 0) {
      glGenBuffers(1, &this->vbo_vertices);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertices);
      glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(this->vertices[0]),
		   this->vertices.data(), GL_STATIC_DRAW);
    }

    if (this->normals.size() > 0) {
      glGenBuffers(1, &this->vbo_normals);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_normals);
      glBufferData(GL_ARRAY_BUFFER, this->normals.size() * sizeof(this->normals[0]),
		   this->normals.data(), GL_STATIC_DRAW);
    }

    if (this->texcoords.size() > 0) {
      glGenBuffers(1, &this->vbo_texcoords);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_texcoords);
      glBufferData(GL_ARRAY_BUFFER, this->texcoords.size() * sizeof(this->texcoords[0]),
		   this->texcoords.data(), GL_STATIC_DRAW);
    }
    
    if (this->tangents.size() > 0) {
      glGenBuffers(1, &this->vbo_tangents);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_tangents);
      glBufferData(GL_ARRAY_BUFFER, this->tangents.size() * sizeof(this->tangents[0]),
		   this->tangents.data(), GL_STATIC_DRAW);
    }
    
    if (this->elements.size() > 0) {
      glGenBuffers(1, &this->ibo_elements);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_elements);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->elements.size() * sizeof(this->elements[0]),
		   this->elements.data(), GL_STATIC_DRAW);
    }
  }

  /**
   * Draw the object
   */
  void draw() {
    if (this->vbo_vertices != 0) {
      glEnableVertexAttribArray(attribute_v_coord);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertices);
      glVertexAttribPointer(
        attribute_v_coord,  // attribute
        4,                  // number of elements per vertex, here (x,y,z,w)
        GL_FLOAT,           // the type of each element
        GL_FALSE,           // take our values as-is
        0,                  // no extra data between each position
        0                   // offset of first element
      );
    }

    if (this->vbo_normals != 0) {
      glEnableVertexAttribArray(attribute_v_normal);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_normals);
      glVertexAttribPointer(
        attribute_v_normal,  // attribute
        3,                   // number of elements per vertex, here (x,y,z)
        GL_FLOAT,            // the type of each element
        GL_FALSE,            // take our values as-is
        0,                   // no extra data between each position
        0                    // offset of first element
      );
    }

    if (this->vbo_texcoords != 0) {
      glEnableVertexAttribArray(attribute_v_texcoords);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_texcoords);
      glVertexAttribPointer(
        attribute_v_texcoords, // attribute
        2,                     // number of elements per vertex, here (x,y)
        GL_FLOAT,              // the type of each element
        GL_FALSE,              // take our values as-is
        0,                     // no extra data between each position
        0                      // offset of first element
      );
    }

    if (this->vbo_tangents != 0) {
      glEnableVertexAttribArray(attribute_v_tangent);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo_tangents);
      glVertexAttribPointer(
        attribute_v_tangent, // attribute
        3,                   // number of elements per vertex, here (x,y,z)
        GL_FLOAT,            // the type of each element
        GL_FALSE,            // take our values as-is
        0,                   // no extra data between each position
        0                    // offset of first element
      );
    }
    
    /* Apply object's transformation matrix */
    glUniformMatrix4fv(uniform_m, 1, GL_FALSE, glm::value_ptr(this->object2world));
    /* Transform normal vectors with transpose of inverse of upper left
       3x3 model matrix (ex-gl_NormalMatrix): */
    glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(glm::mat3(this->object2world)));
    glUniformMatrix3fv(uniform_m_3x3_inv_transp, 1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));
    
    /* Push each element in buffer_vertices to the vertex shader */
    if (this->ibo_elements != 0) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_elements);
      int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
      glDrawElements(GL_TRIANGLES, size/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
    } else {
      glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
    }

    if (this->vbo_tangents != 0)
      glDisableVertexAttribArray(attribute_v_tangent);
    if (this->vbo_texcoords != 0)
      glDisableVertexAttribArray(attribute_v_texcoords);
    if (this->vbo_normals != 0)
      glDisableVertexAttribArray(attribute_v_normal);
    if (this->vbo_vertices != 0)
      glDisableVertexAttribArray(attribute_v_coord);
  }
};
Mesh cube;

int init_resources()
{
  printf("init_resources: %s %s %s\n",
         demos[cur_demo].texture_filename, demos[cur_demo].vshader_filename, demos[cur_demo].fshader_filename);
  // TODO: try to optimize on Android by using ETC1 texture format
  normalmap_id = SOIL_load_OGL_texture
    (
     demos[cur_demo].texture_filename,
     SOIL_LOAD_AUTO,
     SOIL_CREATE_NEW_ID,
     SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
     );
  if (normalmap_id == 0)
    cerr << "SOIL loading error: '" << SOIL_last_result() << "' (" << demos[cur_demo].texture_filename << ")" << endl;

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
  attribute_name = "v_texcoords";
  attribute_v_texcoords = glGetAttribLocation(program, attribute_name);
  if (attribute_v_texcoords == -1) {
    fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
    return 0;
  }
  attribute_name = "v_tangent";
  attribute_v_tangent = glGetAttribLocation(program, attribute_name);
  if (attribute_v_tangent == -1) {
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
  uniform_name = "normalmap";
  uniform_normalmap = glGetUniformLocation(program, uniform_name);
  if (uniform_normalmap == -1) {
    fprintf(stderr, "Warning: Could not bind uniform %s\n", uniform_name);
  }


  // Cube

  // front
  cube.vertices.push_back(glm::vec4(-1.0, -1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0, -1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0,  1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0,  1.0,  1.0,  1.0));
  // top
  cube.vertices.push_back(glm::vec4(-1.0,  1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0,  1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0,  1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0,  1.0, -1.0,  1.0));
  // back
  cube.vertices.push_back(glm::vec4( 1.0, -1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0, -1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0,  1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0,  1.0, -1.0,  1.0));
  // bottom
  cube.vertices.push_back(glm::vec4(-1.0, -1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0, -1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0, -1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0, -1.0,  1.0,  1.0));
  // left
  cube.vertices.push_back(glm::vec4(-1.0, -1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0, -1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0,  1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4(-1.0,  1.0, -1.0,  1.0));
  // right
  cube.vertices.push_back(glm::vec4( 1.0, -1.0,  1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0, -1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0,  1.0, -1.0,  1.0));
  cube.vertices.push_back(glm::vec4( 1.0,  1.0,  1.0,  1.0));

  for (int i = 0; i < 4; i++) { cube.normals.push_back(glm::vec3( 0.0,  0.0,  1.0)); }  // front
  for (int i = 0; i < 4; i++) { cube.normals.push_back(glm::vec3( 0.0,  1.0,  0.0)); }  // top
  for (int i = 0; i < 4; i++) { cube.normals.push_back(glm::vec3( 0.0,  0.0, -1.0)); }  // back
  for (int i = 0; i < 4; i++) { cube.normals.push_back(glm::vec3( 0.0, -1.0,  0.0)); }  // bottom
  for (int i = 0; i < 4; i++) { cube.normals.push_back(glm::vec3(-1.0,  0.0,  0.0)); }  // left
  for (int i = 0; i < 4; i++) { cube.normals.push_back(glm::vec3( 1.0,  0.0,  0.0)); }  // right

  for (int i = 0; i < 6; i++) {
    // front
    cube.texcoords.push_back(glm::vec2(0.0, 0.0));
    cube.texcoords.push_back(glm::vec2(1.0, 0.0));
    cube.texcoords.push_back(glm::vec2(1.0, 1.0));
    cube.texcoords.push_back(glm::vec2(0.0, 1.0));
  }

  // Triangulate
  GLushort cube_elements[] = {
    // front
    0,  1,  2,
    2,  3,  0,
    // top
    4,  5,  6,
    6,  7,  4,
    // back
    8,  9, 10,
    10, 11,  8,
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
  for (int i = 0; i < sizeof(cube_elements)/sizeof(cube_elements[0]); i++)
    cube.elements.push_back(cube_elements[i]);

  cube.compute_tangents();

  cube.upload();
 
  return 1;
}

void free_resources()
{
  glDeleteProgram(program);
  glDeleteTextures(1, &normalmap_id);
}

void logic() {
  float angle = glutGet(GLUT_ELAPSED_TIME) / 1000.0 * glm::radians(10.0);  // 10° per second
  glm::mat4 anim = \
    glm::rotate(glm::mat4(1.0f), angle*3.0f, glm::vec3(1, 0, 0)) *  // X axis
    glm::rotate(glm::mat4(1.0f), angle*2.0f, glm::vec3(0, 1, 0)) *  // Y axis
    glm::rotate(glm::mat4(1.0f), angle*4.0f, glm::vec3(0, 0, 1));   // Z axis

  glm::vec3 object_position = glm::vec3(0.0, 0.0, -2.0);
  glm::mat4 model = glm::translate(glm::mat4(1.0f), object_position);
  glm::mat4 view = glm::lookAt(glm::vec3(0.0, 4.0, 0.0), object_position, glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f*screen_width/screen_height, 0.1f, 10.0f);

  glUseProgram(program);
  cube.object2world = model * anim;

  glUniformMatrix4fv(uniform_v, 1, GL_FALSE, glm::value_ptr(view));
  glm::mat4 v_inv = glm::inverse(view);
  glUniformMatrix4fv(uniform_v_inv, 1, GL_FALSE, glm::value_ptr(v_inv));

  glUniformMatrix4fv(uniform_p, 1, GL_FALSE, glm::value_ptr(projection));

  glutPostRedisplay();
}

void draw()
{
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, normalmap_id);
  glUniform1i(uniform_normalmap, /*GL_TEXTURE*/0);

  cube.draw();
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
  glutCreateWindow("Textures in 3D");

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
