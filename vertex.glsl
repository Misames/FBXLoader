#version 330

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texcoords;

varying vec2 v_texcoords;
varying vec3 v_normal;

uniform float u_time;
uniform mat4 u_scale;
uniform mat4 u_rotation;
uniform mat4 u_translation;
uniform mat4 u_projection;
uniform mat4 u_geometrique;
uniform mat4 u_test;

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
    gl_Position = u_projection * u_translation * u_rotation * u_scale * u_test * pos;
    // gl_Position = u_projection * u_test * pos;
}
