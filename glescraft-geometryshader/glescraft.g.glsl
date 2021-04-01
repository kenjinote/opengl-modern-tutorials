#extension GL_EXT_geometry_shader4 : enable

varying out vec4 texcoord;
varying out vec3 normal;
varying out float intensity;
uniform mat4 mvp;

const vec3 sundir = normalize(vec3(0.5, 1, 0.25));
const float ambient = 0.5;

void main(void) {
	// Two input vertices will be the first and last vertex of the quad
	vec4 a = gl_PositionIn[0];
	vec4 d = gl_PositionIn[1];

	// Save intensity information from second input vertex
	intensity = d.w / 127.0;
	d.w = a.w;

	// Calculate the middle two vertices of the quad
	vec4 b = a;
	vec4 c = a;

	if(a.y == d.y) { // y same
		c.z = d.z;
		b.x = d.x;
	} else { // x or z same
		b.y = d.y;
		c.xz = d.xz;
	}

	// Calculate surface normal
	normal = normalize(cross(a.xyz - b.xyz, b.xyz - c.xyz));

	// Surface intensity depends on angle of solar light
	// This is the same for all the fragments, so we do the calculation in the geometry shader
	intensity *= ambient + (1 - ambient) * clamp(dot(normal, sundir), 0, 1);

	// Emit the vertices of the quad
	texcoord = a; gl_Position = mvp * vec4(a.xyz, 1); EmitVertex();
	texcoord = b; gl_Position = mvp * vec4(b.xyz, 1); EmitVertex();
	texcoord = c; gl_Position = mvp * vec4(c.xyz, 1); EmitVertex();
	texcoord = d; gl_Position = mvp * vec4(d.xyz, 1); EmitVertex();
	EndPrimitive();

	// Double-sided water, so the surface is also visible from below
	if(a.w == 8) {
		texcoord = a; gl_Position = mvp * vec4(a.xyz, 1); EmitVertex();
		texcoord = c; gl_Position = mvp * vec4(c.xyz, 1); EmitVertex();
		texcoord = b; gl_Position = mvp * vec4(b.xyz, 1); EmitVertex();
		texcoord = d; gl_Position = mvp * vec4(d.xyz, 1); EmitVertex();
		EndPrimitive();
	}
}
