#version 330 core

out vec4 FragColor;

in vec3 Color;
in vec2 TextureCoordinates;

//uniform sampler2D Texture; 
void main()
{
   FragColor = vec4(Color, 1.0);
}