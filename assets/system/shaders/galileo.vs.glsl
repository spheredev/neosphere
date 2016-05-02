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

varying vec4 varying_color;
varying vec2 varying_texcoord;

void main()
{
	gl_Position = al_projview_matrix * al_pos;
	varying_color = color;
	varying_texcoord = al_texcoord;
}
