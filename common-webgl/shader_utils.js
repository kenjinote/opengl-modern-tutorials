/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */

'use strict';

function printLog(object) {
    var log = '';

    console.log(object);
    if (object instanceof WebGLShader)
	log = gl.getShaderInfoLog(object);
    else if (object instanceof WebGLProgram)
	log = gl.getProgramInfoLog(object);
    
    console.log(log);
}

function createShader(id, type) {
    var script = document.getElementById(id);
    if (script == undefined) {
	console.error("Error accessing element " + id);
	return false;
    }
    var source = script.text;
    var shader = gl.createShader(type);

    var precision = ''
	+ "#  ifdef GL_FRAGMENT_PRECISION_HIGH \n"
	+ "     precision highp float;         \n"
	+ "#  else                             \n"
	+ "     precision mediump float;       \n"
	+ "#  endif                            \n"
    gl.shaderSource(shader, precision + source);

    gl.compileShader(shader);
    var compileOk = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
    if (!compileOk) {
	console.error('Error in vertex shader\n');
	printLog(shader);
	return false;
    }

    return shader;
}
