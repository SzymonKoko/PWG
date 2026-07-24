#version 450 core

in vec3 f_WorldPos;
in vec3 f_Normal;
in vec3 f_Color;
in vec2 f_Uv;

out vec4 FragColor;

uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform float u_lightIntensity;
uniform vec3 u_cameraPosition;
uniform vec3 u_tint;
uniform int u_hasAlbedo;
uniform sampler2D u_albedo;

void main()
{
    vec3 normal = normalize(f_Normal);
    vec3 lightDir = normalize(-u_lightDirection);
    float ndotl = max(dot(normal, lightDir), 0.0);
    vec3 textureColor = (u_hasAlbedo == 1) ? texture(u_albedo, f_Uv).rgb : vec3(1.0);
    vec3 baseColor = f_Color * textureColor * u_tint;
    vec3 ambient = baseColor * 0.22;
    vec3 diffuse = baseColor * u_lightColor * ndotl * max(u_lightIntensity * 0.00008, 0.35);
    FragColor = vec4(ambient + diffuse, 1.0);
}