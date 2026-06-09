#version 450 core

out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D u_scene;
uniform vec2 u_viewportSize;
uniform float u_exposure;
uniform float u_whitePoint;
uniform float u_bloomStrength;
uniform int u_fxaaEnabled;

vec3 ACESFitted(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 SampleScene(vec2 uv)
{
    return texture(u_scene, clamp(uv, vec2(0.0), vec2(1.0))).rgb;
}

vec3 Bloom(vec2 uv, vec2 texel)
{
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            vec2 offset = vec2(x, y) * texel * 2.0;
            vec3 c = SampleScene(uv + offset);
            float brightness = max(max(c.r, c.g), c.b);
            vec3 bright = max(c - vec3(1.0), vec3(0.0)) * smoothstep(1.0, 4.0, brightness);
            float w = 1.0 / (1.0 + length(vec2(x, y)));
            sum += bright * w;
            weight += w;
        }
    }
    return sum / max(weight, 0.001);
}

vec3 FXAA(vec2 uv, vec2 texel)
{
    vec3 center = SampleScene(uv);
    if (u_fxaaEnabled == 0)
    {
        return center;
    }

    vec3 l = SampleScene(uv + vec2(-texel.x, 0.0));
    vec3 r = SampleScene(uv + vec2( texel.x, 0.0));
    vec3 d = SampleScene(uv + vec2(0.0, -texel.y));
    vec3 u = SampleScene(uv + vec2(0.0,  texel.y));
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float edge = abs(dot(l - r, luma)) + abs(dot(d - u, luma));
    float amount = smoothstep(0.04, 0.18, edge) * 0.45;
    return mix(center, (l + r + d + u) * 0.25, amount);
}

void main()
{
    vec2 texel = 1.0 / max(u_viewportSize, vec2(1.0));
    vec3 hdrColor = FXAA(v_TexCoord, texel);
    hdrColor += Bloom(v_TexCoord, texel) * u_bloomStrength;
    hdrColor *= max(u_exposure, 0.001);

    float whiteScale = max(u_whitePoint, 1.0);
    hdrColor = hdrColor * (1.0 + hdrColor / (whiteScale * whiteScale));
    vec3 mapped = ACESFitted(hdrColor);
    mapped = pow(mapped, vec3(1.0 / 2.2));
    FragColor = vec4(mapped, 1.0);
}
