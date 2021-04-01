varying vec4 texCoords;
uniform sampler2D mytexture;
float cutoff = 0.1;

void main(void) {
    vec2 longitudeLatitude = vec2((atan(texCoords.y, texCoords.x) / 3.1415926 + 1.0) * 0.5,
                                  (asin(texCoords.z) / 3.1415926 + 0.5));

    gl_FragColor = texture2D(mytexture, longitudeLatitude);

    if (gl_FragColor.a < cutoff)
        // alpha value less than user-specified threshold?
    {
        discard; // yes: discard this fragment
    }
}
