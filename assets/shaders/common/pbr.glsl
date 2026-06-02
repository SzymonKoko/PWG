
uniform vec3 u_lightPosition;
uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform float u_lightIntensity;
uniform float u_exposure;
uniform float u_skyAmbient;
uniform float u_groundAmbient;

const float PI = 3.14159265359;

mat3 ComputeTBN(vec3 normal, vec2 uv, vec3 fragPos)
{
    vec3 Q1  = dFdx(fragPos);
    vec3 Q2  = dFdy(fragPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N   = normalize(normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));

    return mat3(T, B, N);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    roughness = clamp(roughness, 0.04, 1.0);
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    roughness = clamp(roughness, 0.04, 1.0);
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ApplyLighting(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float ao, vec3 fragPos)
{
    N = normalize(N);
    V = normalize(V);
    albedo = max(albedo, vec3(0.0));
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);
    ao = clamp(ao, 0.0, 1.0);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    vec3 L = normalize(-u_lightDirection);
    vec3 H = normalize(V + L);
    vec3 radiance = u_lightColor * u_lightIntensity;

    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);        

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    float skyAmount = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyColor = vec3(0.58, 0.68, 0.82) * u_skyAmbient;
    vec3 groundColor = vec3(0.28, 0.24, 0.20) * u_groundAmbient;
    vec3 ambientLight = mix(groundColor, skyColor, skyAmount);
    vec3 ambient = ambientLight * albedo * ao;
    
    vec3 color = ambient + Lo;

    color *= max(u_exposure, 0.01);
    color = color / (color + vec3(1.0));

    return color;

}
