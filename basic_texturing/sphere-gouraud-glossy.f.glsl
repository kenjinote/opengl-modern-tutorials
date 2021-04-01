varying vec3 diffuseColor;
    // the interpolated diffuse Phong lighting
varying vec3 specularColor;
    // the interpolated specular Phong lighting
varying vec4 texCoords;
    // the interpolated texture coordinates
uniform sampler2D mytexture;

void main(void)
{
    vec2 longitudeLatitude = vec2((atan(texCoords.y, texCoords.x) / 3.1415926 + 1.0) * 0.5,
                                  (asin(texCoords.z) / 3.1415926 + 0.5));
    // unusual processing of texture coordinates

    vec4 textureColor = 
        texture2D(mytexture, longitudeLatitude);
    gl_FragColor = vec4(diffuseColor * vec3(textureColor)
        + specularColor * (1.0 - textureColor.a), 1.0);
}
