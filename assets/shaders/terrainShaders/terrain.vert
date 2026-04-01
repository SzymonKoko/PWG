#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normals;
layout (location = 2) in vec2 a_TextureCoordinates;

out vec3 f_Pos;
out vec2 f_TextureCoordinates;
out vec3 f_Normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform sampler2D u_Heightmap;

uniform vec2 u_uvOffset;
uniform vec2 u_uvScale;

void main()
{
    vec2 uv = u_uvOffset + a_TextureCoordinates * u_uvScale;

    //Height
    float h = texture(u_Heightmap, uv).r;

    vec3 pos = a_Pos;
    pos.y = h;

    vec4 worldPos = u_model * vec4(pos, 1.0);
    f_Pos = worldPos.xyz;

    f_TextureCoordinates = uv;

    //Normals
    vec2 texelSize = 1.0 / vec2(textureSize(u_Heightmap, 0));

    float hL = texture(u_Heightmap, uv + vec2(-texelSize.x, 0)).r;
    float hR = texture(u_Heightmap, uv + vec2( texelSize.x, 0)).r;
    float hD = texture(u_Heightmap, uv + vec2(0, -texelSize.y)).r;
    float hU = texture(u_Heightmap, uv + vec2(0,  texelSize.y)).r;

    vec3 normal = normalize(vec3(
        hL - hR,
        2.0,
        hD - hU
    ));

    f_Normal = normal;

    gl_Position = u_projection * u_view * worldPos;
}