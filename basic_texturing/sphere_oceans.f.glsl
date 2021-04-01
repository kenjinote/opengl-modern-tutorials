varying vec4 texCoords;
uniform sampler2D mytexture;

void main(void) {
    vec2 longitudeLatitude = vec2((atan(texCoords.y, texCoords.x) / 3.1415926 + 1.0) * 0.5,
                                  (asin(texCoords.z) / 3.1415926 + 0.5));

    gl_FragColor = texture2D(mytexture, longitudeLatitude);

    if (gl_FragColor.a > 0.5) // opaque 
    {
        //gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); // opaque green
        //gl_FragColor = vec4(0.5 * gl_FragColor.r, 2.0 * gl_FragColor.g, 0.5 * gl_FragColor.b, 1.0);
        gl_FragColor = vec4(0.5 * gl_FragColor.r, 1.0 - 0.5 * (1.0 - gl_FragColor.g), 0.5 * gl_FragColor.b, 1.0);
    }
    else // transparent 
    {
        gl_FragColor = vec4(0.0, 0.0, 0.5, 0.7); // semitransparent dark blue
    }
}
