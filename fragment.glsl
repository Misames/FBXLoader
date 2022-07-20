#version 330

in vec3 v_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_tangent;

struct Material
{
    vec3 AmbientColor;
    vec3 DiffuseColor;
    vec3 SpecularColor;
    float Shininess;
};

layout(std140)
uniform Materials
{
    Material u_Material;
};

uniform vec3 u_CameraPosition;

uniform sampler2D u_DiffuseTexture;

// Projection de texture ---
uniform ProjectorMatrices
{
    mat4 u_ProjectorView;
    mat4 u_ProjectorProjection;
};

uniform sampler2D u_ProjectorTexture;

mat4 biasMatrix = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);

// on va utiliser une variante de texture() : textureProj()
// qui effectue une projection / division 3D -> 2D
// la technique consiste d'abord a convertir les coordonnees
// du point de vue de la lumiere -apres projection- de [-1;+1] vers [0;1]
// y = (x+1)/2 <=> x*0.5 + 0.5
// projector_texcoords = biasMatrix * projector_proj * projector_view * world_position
// ---


// calcul du facteur diffus, suivant la loi du cosinus de Lambert
float Lambert(vec3 N, vec3 L)
{
    return max(0.0, dot(N, L));
}

// cosTheta = le resultat du produit scalaire (N.L, V.H etc...)
vec3 FresnelSchlick(vec3 f0, float cosTheta) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - cosTheta, 5.0);
}

// facteur de normalisation (modele plausible de Lewis)
float NormalisedPhong(float shininess)
{
    return (shininess + 1.0) / 2.0;
}

// calcul du facteur speculaire, methode de Phong
float Phong(vec3 N, vec3 L, vec3 V, float shininess)
{
    // reflexion du vecteur incident I (I = -L)
    // suivant la loi de ibn Sahl - Snell - Descartes
    vec3 R = reflect(-L, N);
    return (dot(N, L) > 0.0) ? pow(max(0.0, dot(R, V)), shininess) : 0.0;
}

// facteur de normalisation (modele plausible de Lewis)
float NormalisedModifiedPhong(float shininess)
{
    return (shininess + 2.0) / 2.0;
}

float ModifiedPhong(vec3 N, vec3 L, vec3 V, float shininess)
{
    // reflexion du vecteur incident I (I = -L)
    // suivant la loi de ibn Sahl - Snell - Descartes
    vec3 R = reflect(-L, N);
    
    return Lambert(N, L) * pow(max(0.0, dot(R, V)), shininess);
}


// facteur de normalisation (modele plausible de Lewis)
float NormalisedBlinnPhong(float shininess)
{
    // approx (shininess + 2.0) / 8.0
    return (shininess + 2.0) / (4.0 * (2.0 - pow(2.0, -shininess/2.0)));
}

// methode de Blinn-Phong
float BlinnPhong(vec3 N, vec3 L, vec3 V, float shininess)
{
    // calcul d'un "half-vector" (vecteur bisecteur)
    vec3 H = normalize(L + V);
    
    return (dot(N, L) > 0.0) ? pow(max(0.0, dot(N, H)), shininess) : 0.0;
}

// methode speculaire de gotanda
vec3 Gotanda(vec3 f0, vec3 N, vec3 L, vec3 V, float shininess)
{
    // calcul d'un "half-vector" (vecteur bisecteur)
    vec3 H = normalize(L + V);
    
    vec3 Fspec = FresnelSchlick(f0, dot(V, H));

    return NormalisedBlinnPhong(shininess) * dot(N,L) * Fspec * pow(max(0.0, dot(N, H)), shininess)
    / (max(dot(N, L), dot(N, V)));
}


void main(void)
{
    // direction de notre lumiere directionnelle (fixe)
    // en repere main droite
    const vec3 L = normalize(vec3(0.0, 0.0, 1.0));
    const vec3 lightColor = vec3(1.0, 1.0, 1.0);
    const float attenuation = 1.0; // on suppose une attenuation faible ici
    // theoriquement, l'attenuation naturelle est proche de 1 / distance�

    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_CameraPosition - v_position);

    vec4 baseTexel = texture2D(u_DiffuseTexture, v_uv);
    // decompression gamma, les couleurs des texels ont ete specifies dans l'espace colorimetrique
    // du moniteur (en sRGB) il faut donc convertir en RGB lineaire pour que les maths soient corrects
    baseTexel.rgb = pow(baseTexel.rgb, vec3(2.2));
    vec3 baseColor = baseTexel.rgb;

    vec3 directColor = vec3(0.0);
    
    float metalness = 1.0;
    float perceptualRoughness = 0.5;
    float roughness = perceptualRoughness * perceptualRoughness;

    {
        vec3 albedo = baseColor * u_Material.DiffuseColor;
        // la couleur diffuse depend du type de materiaux et de l'orientation de la normale
        
        // reflectance moyenne d'un objet "dielectrique" (non-conducteur)
        vec3 f0 = mix(vec3(0.04), albedo, metalness);
        vec3 Fdiff = FresnelSchlick(f0, dot(N, L));
        vec3 diffuseColor = (1.0 - metalness) * albedo * Lambert(N, L);
        float shininess = u_Material.Shininess;
        shininess = (2.0 / (roughness * roughness)) - 2.0;
        vec3 specularColor = mix(vec3(1.0), albedo, metalness) /*u_Material.SpecularColor*/ 
            * NormalisedPhong(shininess) * Gotanda(f0, N, L, V, shininess);
        vec3 R = reflect(-L, N); // note: optimisation possible car deja calculee dans Phong
        // la couleur directe finale est une somme ponderee
        vec3 color = diffuseColor + specularColor;

        directColor += color * lightColor * attenuation;
    }

    // la couleur ambiante traduit une approximation de l'illumination indirecte de l'objet
    vec3 ambientColor = baseColor * u_Material.AmbientColor;
    vec3 indirectColor = ambientColor;

    vec3 color = directColor + indirectColor;
    {
        vec4 projector_texcoords = u_ProjectorProjection * u_ProjectorView * vec4(v_position, 1.0);
        // important : division perspective
        projector_texcoords /= projector_texcoords.w; 
        // inverse l'axe Vertical de la texture
        if (abs(projector_texcoords.x) < 1.0 && abs(projector_texcoords.y) < 1.0) 
        {
            projector_texcoords = biasMatrix * projector_texcoords;
            projector_texcoords.y = 1.0 - projector_texcoords.y;
            vec4 projectorColor = textureProj(u_ProjectorTexture, projector_texcoords);
            color = projectorColor.rgb;
        }
    }

    color = pow(color, vec3(1.0 / 2.2));
    gl_FragColor = vec4(color, 1.0);
}