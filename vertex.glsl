#version 330

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texcoords;
attribute vec4 a_tangent;

varying vec2 v_texcoords;
varying vec3 v_normal;
varying vec4 v_tangent;

uniform float u_time;
uniform mat4 u_projection;
uniform mat4 u_translation;
uniform mat4 u_rotation;
uniform mat4 u_scale;
uniform mat4 u_world;

void main()
{
    v_normal = mat3(u_rotation) * a_normal;
    v_texcoords = a_texcoords;
    v_tangent = a_tangent;
    vec4 pos = vec4(a_position, 1.0);
    gl_Position = u_projection * u_translation * u_rotation * u_scale * u_world * pos;
}
