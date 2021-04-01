attribute vec3 v_coord;
attribute vec2 v_texcoord;
varying vec2 f_texcoord;
uniform mat3 mvp;

void main(void) {
  gl_Position = vec4(mvp * v_coord, 1);
  f_texcoord = v_texcoord;
}
