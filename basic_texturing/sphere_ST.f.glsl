varying vec4 texCoords;
uniform sampler2D mytexture;
uniform vec4 mytexture_ST; // tiling and offset parameters

void main(void) {
    vec2 longitudeLatitude = vec2((atan(texCoords.y, texCoords.x) / 3.1415926 + 1.0) * 0.5,
                                  (asin(texCoords.z) / 3.1415926 + 0.5));

    // Apply tiling and offset
    vec2 texCoordsTransformed = longitudeLatitude * mytexture_ST.xy + mytexture_ST.zw;

    gl_FragColor = texture2D(mytexture, texCoordsTransformed);
}
