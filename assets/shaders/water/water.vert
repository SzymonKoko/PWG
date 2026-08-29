#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normals;
layout (location = 2) in vec2 a_TextureCoordinates;

out vec3 f_WorldPos;
out vec3 f_MacroNormal;
out vec2 f_WorldUV;
out float f_WaterDepth;
out float f_Flow;
out float f_Channel;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform sampler2D u_Heightmap;
uniform sampler2D u_Watermap;
uniform sampler2D u_FlowAccumulationmap;
uniform sampler2D u_RiverChannelmap;
uniform vec2 u_uvOffset;
uniform vec2 u_uvScale;
uniform float u_minVisibleWaterDepth;

float SurfaceAt(vec2 uv)
{
    return texture(u_Heightmap, uv).r + max(texture(u_Watermap, uv).r, 0.0);
}

void main()
{
    vec2 uv = u_uvOffset + a_TextureCoordinates * u_uvScale;
    vec2 texel = 1.0 / vec2(textureSize(u_Heightmap, 0));
    float terrainHeight = texture(u_Heightmap, uv).r;
    float waterDepth = max(texture(u_Watermap, uv).r, 0.0);
    float flow = texture(u_FlowAccumulationmap, uv).r;
    float channel = texture(u_RiverChannelmap, uv).r;

    float center = terrainHeight + waterDepth;
    float left = SurfaceAt(uv - vec2(texel.x, 0.0));
    float right = SurfaceAt(uv + vec2(texel.x, 0.0));
    float down = SurfaceAt(uv - vec2(0.0, texel.y));
    float up = SurfaceAt(uv + vec2(0.0, texel.y));
    float localAverage = (center * 4.0 + left + right + down + up) * 0.125;
    // Ignore the thin rain film produced on every simulation cell. Only pooled
    // water should start leveling into a lake surface.
    float standingWater = smoothstep(u_minVisibleWaterDepth + 0.015, u_minVisibleWaterDepth + 0.10, waterDepth)
        * (1.0 - smoothstep(0.18, 0.65, max(flow, channel)));
    float surfaceHeight = mix(center, localAverage, standingWater * 0.72) + 0.015;

    vec3 position = a_Pos;
    position.y = surfaceHeight;
    vec4 worldPosition = u_model * vec4(position, 1.0);

    f_WorldPos = worldPosition.xyz;
    f_MacroNormal = normalize(mat3(u_model) * normalize(vec3(left - right, 2.0, down - up)));
    f_WorldUV = uv;
    f_WaterDepth = waterDepth;
    f_Flow = flow;
    f_Channel = channel;
    gl_Position = u_projection * u_view * worldPosition;
}
