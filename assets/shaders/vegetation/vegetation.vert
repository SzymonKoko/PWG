#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_Uv;
layout (location = 3) in vec3 a_Color;
layout (location = 4) in vec4 a_InstanceModel0;
layout (location = 5) in vec4 a_InstanceModel1;
layout (location = 6) in vec4 a_InstanceModel2;
layout (location = 7) in vec4 a_InstanceModel3;

out vec3 f_WorldPos;
out vec3 f_Normal;
out vec3 f_Color;
out vec2 f_Uv;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_time;
uniform int u_isGrass;
uniform int u_isTree;
uniform float u_modelMinY;
uniform float u_modelMaxY;

void main()
{
    mat4 model = mat4(a_InstanceModel0, a_InstanceModel1, a_InstanceModel2, a_InstanceModel3);
    vec3 localPosition = a_Pos;
    if (u_isGrass == 1)
    {
        float sourceHeight = max(u_modelMaxY - u_modelMinY, 0.001);
        float bladeHeight = clamp((a_Pos.y - u_modelMinY) / sourceHeight, 0.0, 1.0);
        float phase = dot(model[3].xz, vec2(0.071, 0.053));
        vec2 wind = vec2(sin(u_time * 1.7 + phase), cos(u_time * 1.3 + phase));
        localPosition.xz += wind * sourceHeight * 0.08 * bladeHeight * bladeHeight;
    }
    vec4 worldPosition = model * vec4(localPosition, 1.0);
    if (u_isTree == 1)
    {
        float sourceHeight = max(u_modelMaxY - u_modelMinY, 0.001);
        float crownHeight = smoothstep(0.18, 1.0, clamp((a_Pos.y - u_modelMinY) / sourceHeight, 0.0, 1.0));
        float worldHeight = length(model[1].xyz) * sourceHeight;
        float phase = dot(model[3].xz, vec2(0.017, 0.023));
        float sway = sin(u_time * 0.55 + phase) * 0.72 + sin(u_time * 1.17 + phase * 1.7) * 0.28;
        worldPosition.xz += normalize(vec2(0.82, 0.36)) * sway * worldHeight * 0.012 * crownHeight * crownHeight;
    }
    f_WorldPos = worldPosition.xyz;
    f_Normal = normalize(mat3(transpose(inverse(model))) * a_Normal);
    f_Color = a_Color;
    f_Uv = a_Uv;
    gl_Position = u_projection * u_view * worldPosition;
}
