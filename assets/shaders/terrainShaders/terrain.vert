#version 450 core
layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normals;
layout (location = 2) in vec2 a_TextureCoordinates;
layout (location = 3) in vec3 a_Color;

out vec3 f_Pos;
out vec2 f_TextureCoordinates;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_amplitude;

uniform sampler2D u_Heightmap;


void main()
{
	float h = texture(u_Heightmap, a_TextureCoordinates).r;
	vec3 pos = a_Pos;
	pos.y = h * u_amplitude;

	f_Pos = pos;
	f_TextureCoordinates = a_TextureCoordinates;

	gl_Position = u_projection * u_view * u_model * vec4(pos, 1.0);
}