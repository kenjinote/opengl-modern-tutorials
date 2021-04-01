attribute vec4 coord;

uniform mat4 model;
uniform mat4 cvp;
uniform mat4 lvp;

varying vec3 lightvec;
varying vec4 texcoord;
varying vec4 lightcoord;
varying vec3 normal;
varying vec4 pos;

void main(void) {
	texcoord = coord;
	if(texcoord.w >= 80.0)
		normal = vec3(0.0, 0.0, +1.0);
	else if(texcoord.w >= 64.0)
		normal = vec3(0.0, 0.0, -1.0);
	else if(texcoord.w >= 48.0)
		normal = vec3(0.0, +1.0, 0.0);
	else if(texcoord.w >= 32.0)
		normal = vec3(0.0, -1.0, 0.0);
	else if(texcoord.w >= 16.0)
		normal = vec3(+1.0, 0.0, 0.0);
	else
		normal = vec3(-1.0, 0.0, 0.0);

	pos = model * vec4(coord.xyz, 1);
	lightcoord = lvp * pos;
	gl_Position = cvp * pos;
}
