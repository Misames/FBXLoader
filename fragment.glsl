#version 330

varying vec2 v_texcoords;
varying vec3 v_normal;
varying vec4 v_tangent;

uniform sampler2D u_sampler;

const vec3 L = normalize(vec3(0.0, 0.0, 1.0));

out vec4 o_FragColor;

float Diffuse(const vec3 n, const vec3 l)
{
    return max(dot(n, l), 0.0);
}

float specular(vec3 incident, vec3 normal)
{
    vec3 R = reflect(incident, normal);
    return 0.0;
}

void main()
{
    // La normale originelle du vertex ne nous sert plus que pour creer le repere tangent
    // v_Tangent.w contient la valeur de correction de repère main-droite (handedness)
    vec3 N = normalize(v_normal);
    vec3 T = normalize(v_tangent.xyz);
    vec3 B = normalize(cross(N, T) * v_tangent.w);

    // TBN transforme de Tangent Space vers World Space 
    // (comme v_Tangent et v_Normal ont ete prealablement multipliees par la world matrix
    // TBN est donc la combinaison des transformations tangent-to-local et local-to-world)
    mat3 TBN = mat3(T, B, N);

    // N en tangent space que l'on transform en World Space
    N = texture(u_sampler, v_texcoords).rgb * 2.0 - 1.0;
    N = normalize(TBN * N);

    float lambert = Diffuse(N, L);
    vec4 texcolor = texture2D(u_sampler, v_texcoords);
    o_FragColor = vec4(vec3(lambert), 1.0) * texcolor;
}