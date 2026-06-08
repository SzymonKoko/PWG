#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normals;
layout (location = 2) in vec2 a_TextureCoordinates;

out vec3 f_Pos;
out vec2 f_TextureCoordinates;
out vec3 f_Normal;
out float f_WaterDepth;
out vec2 f_WorldUV;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform sampler2D u_Heightmap;
uniform sampler2D u_Watermap;
uniform sampler2D u_FlowAccumulationmap;
uniform sampler2D u_RiverChannelmap;
uniform bool u_renderWaterOverlay;

uniform vec2 u_uvOffset;
uniform vec2 u_uvScale;

void main()
{
    vec2 uv = u_uvOffset + a_TextureCoordinates * u_uvScale;

    //Height
    float h = texture(u_Heightmap, uv).r;
    float waterDepth = texture(u_Watermap, uv).r;
    float waterVisible = smoothstep(0.008, 0.04, waterDepth);

    vec3 pos = a_Pos;
    pos.y = h;
    if (u_renderWaterOverlay)
    {
        pos.y = h + max(waterDepth, 0.0) + 0.012;
    }

    vec4 worldPos = u_model * vec4(pos, 1.0);
    f_Pos = worldPos.xyz;

    f_TextureCoordinates = uv;
    f_WaterDepth = waterDepth;
    f_WorldUV = uv;

    //Normals
    vec2 texelSize = 1.0 / vec2(textureSize(u_Heightmap, 0));

    float waterNormalScale = u_renderWaterOverlay ? 1.0 : 0.0;
    float hL = texture(u_Heightmap, uv + vec2(-texelSize.x, 0)).r + texture(u_Watermap, uv + vec2(-texelSize.x, 0)).r * waterNormalScale;
    float hR = texture(u_Heightmap, uv + vec2( texelSize.x, 0)).r + texture(u_Watermap, uv + vec2( texelSize.x, 0)).r * waterNormalScale;
    float hD = texture(u_Heightmap, uv + vec2(0, -texelSize.y)).r + texture(u_Watermap, uv + vec2(0, -texelSize.y)).r * waterNormalScale;
    float hU = texture(u_Heightmap, uv + vec2(0,  texelSize.y)).r + texture(u_Watermap, uv + vec2(0,  texelSize.y)).r * waterNormalScale;

    vec3 normal = normalize(vec3(
        hL - hR,
        2.0,
        hD - hU
    ));

    f_Normal = normalize(mix(normal, vec3(0.0, 1.0, 0.0), (u_renderWaterOverlay ? 1.0 : 0.0) * waterVisible * 0.65));

    gl_Position = u_projection * u_view * worldPos;
}
