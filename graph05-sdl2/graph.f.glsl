varying vec4 graph_coord;
uniform vec4 color;

void main(void) {
	float factor;

	if (gl_FrontFacing)
		factor = 1.0;
	else
		factor = 0.5;

	gl_FragColor = (graph_coord / 2.0 + 0.5) * color * factor;
}
