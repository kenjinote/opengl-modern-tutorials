attribute vec2 coord2d;
uniform mat4 p;

void main(void) {
  gl_Position = p * vec4(coord2d, 0.0, 1.0);
}
