#version 450 core

layout (location = 0) in vec3 a_Pos;

out vec3 f_WorldPos;
out vec3 f_MacroNormal;
out vec2 f_WorldUV;
out float f_RenderLod;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform sampler2D u_WaterSurfaceCache;
uniform vec2 u_clipmapCenter;
uniform float u_gridSpacing;
uniform float u_worldSize;
uniform int u_ringLevel;

float SurfaceAt(vec2 uv, float lod)
{
    return textureLod(u_WaterSurfaceCache, clamp(uv, vec2(0.0), vec2(1.0)), lod).r;
}

void main()
{
    vec2 local = a_Pos.xz;
    float edgeDistance = max(abs(local.x), abs(local.y));
    float morph = u_ringLevel < 5 ? smoothstep(56.0, 64.0, edgeDistance) : 0.0;
    vec2 coarseLocal = round(local * 0.5) * 2.0;
    local = mix(local, coarseLocal, morph);

    vec2 worldXZ = u_clipmapCenter + local * u_gridSpacing;
    float halfWorld = (u_worldSize - 1.0) * 0.5;
    vec2 uv = (worldXZ + vec2(halfWorld)) / max(u_worldSize - 1.0, 1.0);
    float lod = float(u_ringLevel);
    vec2 texel = vec2(exp2(lod) / u_worldSize);
    float center = SurfaceAt(uv, lod);
    float left = SurfaceAt(uv - vec2(texel.x, 0.0), lod);
    float right = SurfaceAt(uv + vec2(texel.x, 0.0), lod);
    float down = SurfaceAt(uv - vec2(0.0, texel.y), lod);
    float up = SurfaceAt(uv + vec2(0.0, texel.y), lod);

    f_WorldPos = vec3(worldXZ.x, center + 0.015, worldXZ.y);
    f_MacroNormal = normalize(vec3(left - right, 2.0 * u_gridSpacing, down - up));
    f_WorldUV = uv;
    f_RenderLod = lod;
    gl_Position = u_projection * u_view * vec4(f_WorldPos, 1.0);
}
