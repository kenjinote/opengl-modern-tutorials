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

varying vec3 diffuseColor;
  // the diffuse Phong lighting computed in the vertex shader
varying vec3 specularColor;
  // the specular Phong lighting computed in the vertex shader
varying vec4 texCoords; // the texture coordinates

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
  vec4(0.0,  1.0,  2.0, 1.0),
  vec4(1.0,  1.0,  1.0, 1.0),
  vec4(1.0,  1.0,  1.0, 1.0),
  0.0, 1.0, 0.0,
  180.0, 0.0,
  vec3(0.0, 0.0, 0.0)
);
vec4 scene_ambient = vec4(0.2, 0.2, 0.2, 1.0);

struct material
{
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  float shininess;
};
material mymaterial = material(
  vec4(0.2, 0.2, 0.2, 1.0),
  vec4(1.0, 0.8, 0.8, 1.0),
  vec4(1.0, 1.0, 1.0, 1.0),
  5.0
);

void main(void)
{
  vec4 v_coord4 = vec4(v_coord, 1.0);
  mat4 mvp = p*v*m;
  vec3 normalDirection = normalize(m_3x3_inv_transp * v_normal);
  vec3 viewDirection = normalize(vec3(v_inv * vec4(0.0, 0.0, 0.0, 1.0) - m * v_coord4));
  vec3 lightDirection;
  float attenuation;

  if (light0.position.w == 0.0) // directional light
    {
      attenuation = 1.0; // no attenuation
      lightDirection = normalize(vec3(light0.position));
    }
  else // point or spot light (or other kind of light)
    {
      vec3 vertexToLightSource = vec3(light0.position - m * v_coord4);
      float distance = length(vertexToLightSource);
      lightDirection = normalize(vertexToLightSource);
      attenuation = 1.0 / (light0.constantAttenuation
                           + light0.linearAttenuation * distance
                           + light0.quadraticAttenuation * distance * distance);

      if (light0.spotCutoff <= 90.0) // spotlight
        {
          float clampedCosine = max(0.0, dot(-lightDirection, normalize(light0.spotDirection)));
          if (clampedCosine < cos(radians(light0.spotCutoff))) // outside of spotlight cone
            {
              attenuation = 0.0;
            }
          else
            {
              attenuation = attenuation * pow(clampedCosine, light0.spotExponent);
            }
        }
    }

  vec3 ambientLighting = vec3(scene_ambient);
    // without material color!

  vec3 diffuseReflection = attenuation
    * vec3(light0.diffuse)
    * max(0.0, dot(normalDirection, lightDirection));
    // without material color!

  vec3 specularReflection;
  if (dot(normalDirection, lightDirection) < 0.0) // light source on the wrong side?
    {
      specularReflection = vec3(0.0, 0.0, 0.0); // no specular reflection
    }
  else // light source on the right side
    {
      specularReflection = attenuation * vec3(light0.specular) * vec3(mymaterial.specular)
        * pow(max(0.0, dot(reflect(-lightDirection, normalDirection), viewDirection)),
              mymaterial.shininess);
    }

  diffuseColor = ambientLighting + diffuseReflection;
  specularColor = specularReflection;
  texCoords = v_coord4;
  gl_Position = mvp * v_coord4;
}
