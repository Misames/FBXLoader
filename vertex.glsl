#version 330

in vec3 a_position;
in vec3 a_normal;
in vec2 a_uv;
in vec4 a_tangent;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_uv;
out vec4 v_tangent;

uniform mat4 u_translation;
uniform mat4 u_rotation;
uniform mat4 u_scale;
uniform mat4 u_world;

uniform Matrices
{
    mat4 u_view;
    mat4 u_projection;
};

void main()
{
    v_uv = a_uv;
    v_position = a_position;
    v_tangent = a_tangent;
    mat3 NormalMatrix = transpose(inverse(mat3(u_world)));
    v_normal = NormalMatrix * a_normal;
    gl_Position = u_projection * u_translation * u_rotation * u_scale * u_world * vec4(a_position, 1.0);
}
