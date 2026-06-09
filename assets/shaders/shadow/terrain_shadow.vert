#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 2) in vec2 a_TextureCoordinates;

uniform mat4 u_model;
uniform mat4 u_lightSpaceMatrix;
uniform sampler2D u_Heightmap;
uniform vec2 u_uvOffset;
uniform vec2 u_uvScale;

void main()
{
    vec2 uv = u_uvOffset + a_TextureCoordinates * u_uvScale;
    vec3 pos = a_Pos;
    pos.y = texture(u_Heightmap, uv).r;
    gl_Position = u_lightSpaceMatrix * u_model * vec4(pos, 1.0);
}
