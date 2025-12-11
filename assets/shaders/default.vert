#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 2) in vec2 aTextureCoordinates;
layout (location = 3) in vec3 aColor;

uniform sampler2D heightmap;
uniform float amplitude = 0.01;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

float realHeight;

out vec3 Color;
out vec2 TextureCoordinates;
out float Height;
out vec3 normal;

void main()
{
   float h = texture(heightmap, aTextureCoordinates).r;
   realHeight = h * amplitude;
   gl_Position = projection * view * model * vec4(aPos.x, realHeight, aPos.z, 1.0);
   Color = aColor;
   TextureCoordinates = aTextureCoordinates; 
   Height = realHeight;
   

   //Calculating normals
   vec2 texel = 1.0 / textureSize(heightmap, 0);

	float hL = texture(heightmap, aTextureCoordinates - vec2(texel.x, 0)).r;
	float hR = texture(heightmap, aTextureCoordinates + vec2(texel.x, 0)).r;
	float hD = texture(heightmap, aTextureCoordinates - vec2(0, texel.y)).r;
	float hU = texture(heightmap, aTextureCoordinates + vec2(0, texel.y)).r;

	vec3 dx = vec3(1.0, (hR - hL) * amplitude, 0.0);
	vec3 dz = vec3(0.0, (hU - hD) * amplitude, 1.0);

	normal = normalize(cross(dz, dx));
}