varying vec2 f_texcoord;
uniform sampler2D mypalette;
uniform sampler2D mytexture;
uniform int colorkey;

void main(void) {
  vec4 index = texture2D(mytexture, f_texcoord);
  if (int(index.r*255.0) == colorkey)
    gl_FragColor = vec4(0,0,0,0);
  else
    gl_FragColor = texture2D(mypalette, vec2(index.r, 0.5));
}
