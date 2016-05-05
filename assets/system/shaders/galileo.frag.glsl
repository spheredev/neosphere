/**
 *  minisphere Galileo default fragment shader
 *  provides the basic guarantees required by the Galileo specification
 *  (c) 2015-2016 Fat Cerberus
**/

#ifdef GL_ES
precision mediump float;
#endif

// texturing parameters courtesy of Allegro
uniform sampler2D al_tex;
uniform bool al_use_tex;

// input from vertex shader
varying vec4 varying_color;
varying vec2 varying_texcoord;

void main()
{
	gl_FragColor = al_use_tex
		? varying_color * texture2D(al_tex, varying_texcoord)
        : varying_color;
}
