#version 450 core

#engine_include "../common/lighting.glsl"

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;

uniform sampler2D u_Splatmap;
uniform sampler2D u_Normalmap;
uniform sampler2DArray u_Textures;

void main()
{

	vec4 splat = texture(u_Splatmap, f_TextureCoordinates);
	vec3 normal = normalize(texture(u_Normalmap, f_TextureCoordinates * 0.05).rgb * 2.0 - 1.0);
	vec3 detailedNormal = normalize(texture(u_Normalmap, f_TextureCoordinates * 5.0).rgb * 2.0 - 1.0);
	detailedNormal.xy *= 0.2;

	vec3 color = vec3(0.0f);

	vec2 worldUV = f_Pos.xz;

	vec2 grassUV = worldUV * 0.15;
	vec2 dirtUV  = worldUV * 0.10;
	vec2 rockUV  = worldUV * 0.05;
	vec2 sandUV  = worldUV * 0.20;

	color += texture(u_Textures, vec3(grassUV, 0)).rgb * splat.r;
	color += texture(u_Textures, vec3(dirtUV,  1)).rgb * splat.g;
	color += texture(u_Textures, vec3(rockUV,  2)).rgb * splat.b;
	color += texture(u_Textures, vec3(sandUV,  3)).rgb * splat.a;

	vec3 finalNormal = normalize(normal + detailedNormal);
	vec3 finalColor = ApplyLighting(finalNormal, color);

	FragColor = vec4(finalColor, 1.0f);
}