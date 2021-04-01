/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Martin Kraus, Sylvain Beucler
 */
attribute vec4 v_coord;
attribute vec3 v_normal;
attribute vec2 v_texcoords;
attribute vec3 v_tangent;
uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
varying vec4 position;  // position of the vertex (and fragment) in world space
varying vec2 texCoords;
varying mat3 localSurface2World; // mapping from local surface coordinates to world coordinates

void main()
{
  mat4 mvp = p*v*m;
  position = m * v_coord;

  // the signs and whether tangent is in localSurface2View[1] or
  // localSurface2View[0] depends on the tangent attribute, texture
  // coordinates, and the encoding of the normal map
  localSurface2World[0] = normalize(vec3(m * vec4(v_tangent, 0.0)));
  localSurface2World[2] = normalize(m_3x3_inv_transp * v_normal);
  localSurface2World[1] = normalize(cross(localSurface2World[2], localSurface2World[0]));

  texCoords = v_texcoords;
  gl_Position = mvp * v_coord;
}
