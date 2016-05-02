/**
 *  minisphere Galileo default vertex shader
 *  provides the basic guarantees required by the Galileo specification
 *  (c) 2015-2016 Fat Cerberus
**/

// when adapting this shader, the uniforms and attributes below need to
// be kept.  if you remove them, you won't be able to make a useful
// shader for minisphere.
attribute vec4 al_pos;
attribute vec4 al_color;
attribute vec2 al_texcoord;
uniform mat4 al_projview_matrix;
uniform mat4 colormatrix;

varying vec4 varying_color;
varying vec2 varying_texcoord;

void main()
{
	// set alpha = 1.0 before applying the color matrix so that alpha
	// doesn't influence translations.  this is about the best we can do since
	// OpenGL doesn't support 5x5 matrices.
	vec4 color = vec4(al_color.rgb, 1.0);
	float alpha = al_color.a;
	color = colormatrix * color; color.a = alpha;

	// calculate vertex and set up varyings for the fragment shader
	gl_Position = al_projview_matrix * al_pos;
	varying_color = color;
	varying_texcoord = al_texcoord;
}
