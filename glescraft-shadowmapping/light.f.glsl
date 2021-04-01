uniform sampler2D texture;
varying vec4 texcoord;

void main(void) {
	vec2 coord2d;

	if(texcoord.w >= 32.0 && texcoord.w < 64.0)
		coord2d = vec2((fract(texcoord.x) + texcoord.w) / 16.0, texcoord.z);
	else
		coord2d = vec2((fract(texcoord.x + texcoord.z) + texcoord.w) / 16.0, -texcoord.y);

	vec4 color = texture2D(texture, coord2d);

	// Very cheap "transparency": don't draw pixels with a low alpha value
	if(color.a < 0.4)
		discard;
}
