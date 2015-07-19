#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform bool al_use_tex;
varying vec4 varying_color;
varying vec2 varying_texcoord;

void main()
{
	if (al_use_tex)
		gl_FragColor = varying_color * texture2D(al_tex, varying_texcoord);
	else
		gl_FragColor = varying_color;
}
