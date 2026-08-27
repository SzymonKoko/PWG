#version 450 core

in vec2 f_Uv;

uniform sampler2D u_albedo;
uniform int u_hasAlbedo;

void main()
{
    if (u_hasAlbedo == 1 && texture(u_albedo, f_Uv).a < 0.35)
    {
        discard;
    }
}
