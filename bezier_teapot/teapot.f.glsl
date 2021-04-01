varying vec3 f_color;

void main(void) {
  gl_FragColor = vec4(f_color.xyz, 1.0);
}
