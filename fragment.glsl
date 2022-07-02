#version 330

varying vec3 v_normal;
varying vec2 v_texcoords;

uniform sampler2D u_sampler;

// Equation de Lambert
// n = normale au fragment
// l = direction VERS la lumiere
float Diffuse(const vec3 n, const vec3 l)
{
    return max(dot(n, l), 0.0);
}

float specular(vec3 incident, vec3 normal)
{
    vec3 R = reflect(incident, normal);
    return 0;
    // return max(dot(R, V), 0.0);
    // max((R.V), 0) * Is * Ks
    //
}

const vec3 L = normalize(vec3(0.0, 0.0, 1.0));
// uniform Light u_light;
// uniform Material u_material;

void main(void)
{
    // v_normal a "subi" un lerp, il faut renormaliser le vecteur
    vec3 N = normalize(v_normal);
    float lambert = Diffuse(N, L);
    vec4 texcolor = texture2D(u_sampler, v_texcoords);
    gl_FragColor = vec4(vec3(lambert), 1.0) * texcolor;
}
