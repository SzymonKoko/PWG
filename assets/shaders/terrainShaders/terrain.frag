#version 450 core

#engine_include "../common/pbr.glsl"

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;
in vec3 f_Normal;

uniform sampler2DArray u_Textures;
uniform vec3 u_cameraPosition;

vec2 ParallaxMapping(float height, vec2 texCoords, vec3 viewDirection)
{
    float denom = max(viewDirection.z, 0.001);
    vec2 p = viewDirection.xy / denom * (height);
    return texCoords - p;
}

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

vec2 hash2(vec2 p)
{
    float n = hash(p);
    return vec2(n, hash(p + n));
}

vec2 rotate(vec2 uv, float a)
{
    float s = sin(a);
    float c = cos(a);
    return mat2(c,-s,s,c) * uv;
}

vec3 StochasticSample(sampler2DArray tex, vec2 uv, int layer)
{
    const float k = 1.7320508; // sqrt(3)

    vec2 skewed = vec2(
        uv.x + uv.y * 0.5,
        uv.y * k * 0.5
    );

    vec2 base = floor(skewed);
    vec2 f = fract(skewed);

    float w1 = 1.0 - f.x - f.y;
    float w2 = f.x;
    float w3 = f.y;

    vec2 o1 = hash2(base);
    vec2 o2 = hash2(base + vec2(1,0));
    vec2 o3 = hash2(base + vec2(0,1));

    vec3 s1 = texture(tex, vec3(rotate(uv + o1, o1.x * 6.28), layer)).rgb;
	vec3 s2 = texture(tex, vec3(rotate(uv + o2, o2.x * 6.28), layer)).rgb;
	vec3 s3 = texture(tex, vec3(rotate(uv + o3, o3.x * 6.28), layer)).rgb;

    //return s1*0.6 + s2*0.25 + s3*0.15;
    return s1*w1 + s2*w2 + s3*w3;
}

void main()
{
    vec3 normal = normalize(f_Normal);

    vec2 uv = f_Pos.xz * 0.01;

    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);
    float rock = smoothstep(0.05, 0.95, slope);

    mat3 TBN = ComputeTBN(normal, uv, f_Pos);
    vec3 V = normalize(u_cameraPosition - f_Pos);
    vec3 viewDirTS = normalize(transpose(TBN) * V);

    float gH = texture(u_Textures, vec3(uv, 5)).r;
    float rH = texture(u_Textures, vec3(uv, 10)).r;
    float height = mix(gH, rH, rock);

    float dist = length(u_cameraPosition - f_Pos);
    float fade = clamp(1.0 - dist / 50.0, 0.0, 1.0);

    vec2 parallaxUV = ParallaxMapping(height, uv, viewDirTS);
    uv = mix(uv, parallaxUV, fade);
    
    // GRASS
    vec3 gA = texture(u_Textures, vec3(uv, 0)).rgb;
    vec3 gN = texture(u_Textures, vec3(uv, 1)).rgb * 2.0 - 1.0;
    float gR = texture(u_Textures, vec3(uv, 2)).r;
    float gM = texture(u_Textures, vec3(uv, 3)).r;
    float gAO = texture(u_Textures, vec3(uv, 4)).r;
    

    // ROCK
    vec3 rA = texture(u_Textures, vec3(uv, 5)).rgb;
    vec3 rN = texture(u_Textures, vec3(uv, 6)).rgb * 2.0 - 1.0;
    float rR = texture(u_Textures, vec3(uv, 7)).r;
    float rM = texture(u_Textures, vec3(uv, 8)).r;
    float rAO = texture(u_Textures, vec3(uv, 9)).r;
    

    // BLEND
    vec3 albedo = mix(gA, rA, rock);
    vec3 normalTex = normalize(mix(gN, rN, rock));
    float roughness = mix(gR, rR, rock);
    float metallic = mix(gM, rM, rock);
    float ao = mix(gAO, rAO, rock);
    

    vec3 N = normal;//normalize(TBN * normalTex);    

    vec3 finalColor = ApplyLighting(N, V, albedo, metallic, roughness, ao, f_Pos);

    FragColor = vec4(finalColor, 1.0);
}