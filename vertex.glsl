#version 330
attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texcoords;

varying vec2 v_texcoords;

// LES NORMALES SE TRANSFORMENT AVEC LA WORLD MATRIX
// SSI LA WORLD MATRIX NE CONTIENT PAS DE DEFORMATION
varying vec3 v_normal;

// uniform = constantes pour la frame
uniform float u_time;

uniform mat4 u_scale;
uniform mat4 u_rotation;
uniform mat4 u_translation;
uniform mat4 u_projection;

void main(void) 
{
	// on force la matrice en mat3
	// attention! on ne doit pas tenir compte de la translation 
	// (et si possible eviter le scale egalement)
	// autrement il faudrait faire ceci:
	// transpose ( inverse ( mat3(worldMatrix) ) )
	v_normal = mat3(u_rotation) * a_normal;

	v_texcoords = a_texcoords;
	vec4 pos = vec4(a_position, 1.0);

	gl_Position = u_projection * u_translation 
					* u_rotation * u_scale * pos;
}
