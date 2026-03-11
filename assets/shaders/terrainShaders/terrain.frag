#version 450 core

#engine_include "../common/lighting.glsl"

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;

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

    return s1*w1 + s2*w2 + s3*w3;
}

void main()
{
    vec3 normal = normalize(texture(u_Normalmap, f_TextureCoordinates).rgb * 2.0 - 1.0);

    vec2 worldUV = f_Pos.xz;

    // ===== TERRAIN SCALE =====

    vec2 grassUV = worldUV * 0.005;
    vec2 rockUV  = worldUV * 0.03;

    // ===== DETAIL SCALE =====

    vec2 grassDetailUV = worldUV * 0.15;
    vec2 rockDetailUV  = worldUV * 0.2;

    // ===== UV DISTORTION =====

    vec2 noise = texture(u_Normalmap, worldUV * 0.002).rg;
    noise = noise * 2.0 - 1.0;

    grassUV += noise * 0.3;
    rockUV  += noise * 0.3;

    // ===== SLOPE BLENDING =====

    float slope = texture(u_Normalmap, f_TextureCoordinates).a;

    float stone = smoothstep(0.3, 0.6, slope);
    float grass = 1.0 - stone;

    // ===== STOCHASTIC MACRO TEXTURE =====

    vec3 grassMacro = StochasticSample(u_Textures, grassUV, 0);
    vec3 rockMacro  = StochasticSample(u_Textures, rockUV, 2);

    // ===== DETAIL TEXTURE =====

    vec3 grassDetail = texture(u_Textures, vec3(grassDetailUV, 0)).rgb;
    vec3 rockDetail  = texture(u_Textures, vec3(rockDetailUV, 2)).rgb;

    // ===== MACRO COLOR VARIATION =====

    float macro = texture(u_Normalmap, worldUV * 0.001).r;
    macro = 0.7 + macro * 0.6;

    // ===== FINAL MATERIAL COLORS =====

    vec3 grassColor = grassMacro * macro * (0.7 + grassDetail * 0.3);
    vec3 rockColor  = rockMacro  * macro * (0.7 + rockDetail  * 0.3);

    vec3 color = vec3(0.0);

    color += grassColor * grass;
    color += rockColor * stone;

    vec3 finalColor = ApplyLighting(normal, color, f_Pos);

    FragColor = vec4(finalColor, 1.0);
}