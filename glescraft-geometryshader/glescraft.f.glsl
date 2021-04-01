varying vec4 texcoord;
varying vec3 normal;
varying float intensity;
uniform sampler3D texture;

const vec4 fogcolor = vec4(0.6, 0.8, 1.0, 1.0);
const float fogdensity = .00003;

void main(void) {
	vec4 color;

	// If the texture index is negative, it is a top or bottom face, otherwise a side face
	// Side faces are less bright than top faces, simulating a sun at noon
	if(normal.y != 0) {
		color = texture3D(texture, vec3(texcoord.x, texcoord.z, (texcoord.w + 0.5) / 16.0));
	} else {
		color = texture3D(texture, vec3(texcoord.x + texcoord.z, -texcoord.y, (texcoord.w + 0.5) / 16.0));
	}
	
	// Very cheap "transparency": don't draw pixels with a low alpha value
	if(color.a < 0.4)
		discard;

	// Attenuate
	color *= intensity;

	// Calculate strength of fog
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = clamp(exp(-fogdensity * z * z), 0.2, 1);

	// Final color is a mix of the actual color and the fog color
	gl_FragColor = mix(fogcolor, color, fog);
}
