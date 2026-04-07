#version 450 core

#engine_include "../common/lighting.glsl"

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;
in vec3 f_Normal;

uniform sampler2D u_Normalmap;
uniform sampler2DArray u_Textures;

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

    // proste UV (jednolita skala na cały teren)
    vec2 uv = f_Pos.xz * 0.02;

    // slope (0 = płasko, 1 = stromo)
    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

    // blend między trawą a skałą
    float rock = smoothstep(0.15, 0.85, slope);
    float grass = 1.0 - rock;

    // próbki tekstur (bez żadnych cudów)
    vec3 grassColor = texture(u_Textures, vec3(uv, 0)).rgb;
    vec3 rockColor  = texture(u_Textures, vec3(uv, 2)).rgb;

    // miks
    vec3 color = grassColor * grass + rockColor * rock;

    vec3 finalColor = ApplyLighting(normal, color, f_Pos);

    FragColor = vec4(finalColor, 1.0);
}