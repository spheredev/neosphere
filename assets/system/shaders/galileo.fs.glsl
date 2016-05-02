/**
 *  minisphere Galileo default fragment shader
 *  provides the basic guarantees required by the Galileo specification
 *  (c) 2015-2016 Fat Cerberus
**/

#ifdef GL_ES
precision mediump float;
#endif

// when adapting this shader, the uniforms below need to be kept.  if you
// remove them, you won't be able to make a useful shader for minisphere.
uniform sampler2D al_tex;
uniform bool al_use_tex;
uniform mat4 colormatrix;

varying vec4 varying_color;
varying vec2 varying_texcoord;

void main()
{
	vec4 color = al_use_tex
        ? varying_color * texture2D(al_tex, varying_texcoord)
        : varying_color;

	// set alpha = 1.0 before applying the color matrix so that alpha doesn't
	// skew translations.  this is about the best we can do since OpenGL
	// doesn't support 5x5 matrices.
    float old_alpha = color.a;
    color.a = 1.0;
    color = colormatrix * color;
    color.a = old_alpha;

    gl_FragColor = color;
}
