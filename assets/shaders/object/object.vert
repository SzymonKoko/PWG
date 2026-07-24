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

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform int u_instanced;

void main()
{
    mat4 instanceModel = mat4(a_InstanceModel0, a_InstanceModel1, a_InstanceModel2, a_InstanceModel3);
    mat4 model = (u_instanced == 1) ? instanceModel : u_model;
    vec4 worldPos = model * vec4(a_Pos, 1.0);
    f_WorldPos = worldPos.xyz;
    f_Normal = normalize(mat3(transpose(inverse(model))) * a_Normal);
    f_Color = a_Color;
    f_Uv = a_Uv;
    gl_Position = u_projection * u_view * worldPos;
}
