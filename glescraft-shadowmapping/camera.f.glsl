varying vec4 texcoord;
varying vec4 lightcoord;
varying vec3 normal;
uniform vec3 lightpos;
uniform float depth_offset;
uniform sampler2D texture;
uniform sampler2D shadowmap;

varying vec4 pos;
const vec4 fogcolor = vec4(0.6, 0.8, 1.0, 1.0);
const float fogdensity = .00003;

void main(void) {
	vec2 coord2d;
	float intensity = 0.85;

	// If the texture index is negative, it is a top or bottom face, otherwise a side face
	// Side faces are less bright than top faces, simulating a sun at noon
	if(normal.y != 0.0) {
		coord2d = vec2((fract(texcoord.x) + texcoord.w) / 16.0, texcoord.z);
	} else {
		coord2d = vec2((fract(texcoord.x + texcoord.z) + texcoord.w) / 16.0, -texcoord.y);
	}

	vec4 color = texture2D(texture, coord2d);

	// Very cheap "transparency": don't draw pixels with a low alpha value
	if(color.a < 0.4)
		discard;

	vec3 lightdir = normalize(lightpos - pos.xyz);
	intensity *= clamp(dot(normal, lightdir), 0.0, 1.0);


	vec4 lightcoorddiv = lightcoord;
	lightcoorddiv.z += depth_offset;
	lightcoorddiv /= lightcoord.w;

	// If the depth found in the shadow map is less than that of this fragment,
	// something else along the same ray of light is closer to the light source,
	// so we are in the shadow.
	
	// Uncomment to prevent everything outside the light source's FOV being in the shadow.
	// if(all(equal(lightcoorddiv.xy, clamp(lightcoorddiv.xy, 0, 1))))

	if(texture2D(shadowmap, lightcoorddiv.xy).z < lightcoorddiv.z)
		intensity = 0;

	// Attenuate sides of blocks
	color.xyz *= intensity + 0.15;

	// Calculate strength of fog
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = clamp(exp(-fogdensity * z * z), 0.2, 1.0);

	// Final color is a mix of the actual color and the fog color
	gl_FragColor = mix(fogcolor, color, fog);
}
