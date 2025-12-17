#version 450 core

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;

uniform sampler2D u_Splatmap;
uniform sampler2DArray u_Textures;

void main()
{

	vec4 splat = texture(u_Splatmap, f_TextureCoordinates);

	vec3 color = vec3(0.0f);
	vec2 scaledUVs = f_TextureCoordinates * 1.0f;
	
	color += texture(u_Textures, vec3(scaledUVs, 0)).rgb * splat.r;
	color += texture(u_Textures, vec3(scaledUVs, 1)).rgb * splat.g;
	color += texture(u_Textures, vec3(scaledUVs, 2)).rgb * splat.b;
	color += texture(u_Textures, vec3(scaledUVs, 3)).rgb * splat.a;

	FragColor = vec4(color, 1.0f);
}