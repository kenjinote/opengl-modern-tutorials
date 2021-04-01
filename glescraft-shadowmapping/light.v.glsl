attribute vec4 coord;
varying vec4 texcoord;
uniform mat4 model;
uniform mat4 lvp;

void main(void) {
	texcoord = coord;
	gl_Position = lvp * model * vec4(coord.xyz, 1);
}
