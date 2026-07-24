#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_Uv;
layout (location = 3) in vec3 a_Color;

out vec3 f_WorldPos;
out vec3 f_Normal;
out vec3 f_Color;
out vec2 f_Uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    vec4 worldPos = u_model * vec4(a_Pos, 1.0);
    f_WorldPos = worldPos.xyz;
    f_Normal = normalize(mat3(transpose(inverse(u_model))) * a_Normal);
    f_Color = a_Color;
    f_Uv = a_Uv;
    gl_Position = u_projection * u_view * worldPos;
}
