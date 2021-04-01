/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Martin Kraus, Sylvain Beucler
 */
attribute vec3 v_coord;
attribute vec3 v_normal;
uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

varying float levelOfLighting; // the level of diffuse
  // lighting that is computed in the vertex shader
varying vec4 texCoords;

struct lightSource
{
  vec4 position;
  vec4 diffuse;
  vec4 specular;
  float constantAttenuation, linearAttenuation, quadraticAttenuation;
  float spotCutoff, spotExponent;
  vec3 spotDirection;
};
lightSource light0 = lightSource(
  vec4(2.0,  1.0,  1.0, 0.0),
  vec4(1.0,  1.0,  1.0, 1.0),
  vec4(1.0,  1.0,  1.0, 1.0),
  0.0, 1.0, 0.0,
  180.0, 0.0,
  vec3(0.0, 0.0, 0.0)
);

void main(void)
{
  vec4 v_coord4 = vec4(v_coord, 1.0);
  mat4 mvp = p*v*m;
  vec3 normalDirection = normalize(m_3x3_inv_transp * v_normal);
  vec3 viewDirection = normalize(vec3(v_inv * vec4(0.0, 0.0, 0.0, 1.0) - m * v_coord4));
  vec3 lightDirection;

  if (light0.position.w == 0.0) // directional light
    {
      lightDirection = normalize(vec3(light0.position));
    }
  else // point or spot light (or other kind of light)
    {
      vec3 vertexToLightSource = vec3(light0.position - m * v_coord4);
      lightDirection = normalize(vertexToLightSource);
    }

  levelOfLighting = max(0.0, dot(normalDirection, lightDirection));
  texCoords = v_coord4;
  gl_Position = mvp * v_coord4;
}
