varying float levelOfLighting;
varying vec4 texCoords;
uniform sampler2D mytexture;
uniform sampler2D mytexture_sunlit;

void main(void)
{
    vec2 longitudeLatitude = vec2((atan(texCoords.y, texCoords.x) / 3.1415926 + 1.0) * 0.5,
                                  (asin(texCoords.z) / 3.1415926 + 0.5));

    vec4 nighttimeColor = texture2D(mytexture, longitudeLatitude);
    vec4 daytimeColor = texture2D(mytexture_sunlit, longitudeLatitude);

    gl_FragColor = mix(nighttimeColor, daytimeColor, levelOfLighting);
}
