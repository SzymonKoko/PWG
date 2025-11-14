#version 430 core

out vec4 FragColor;

in vec3 Color;
in vec2 TextureCoordinates;
in float Height;
in vec3 normal;

// Kolory materia³ów (na razie sta³e – póŸniej zrobisz tekstury)
vec3 grass = vec3(0.14, 0.26, 0.08);
vec3 rock  = vec3(0.57, 0.57, 0.57);
vec3 snow  = vec3(1.0, 1.0, 1.0);

uniform float minHeight = 0.0;
uniform float maxHeight = 600.0;

void main()
{
    // Wysokoœæ wystawiona z vertex shadera
    float heightNorm = clamp((Height - minHeight) / (maxHeight - minHeight), 0.0, 1.0);

    // Slope na podstawie normalnej
    float slope = dot(normalize(normal), vec3(0, 1, 0)); 
    slope = clamp(slope, 0.0, 1.0);

    // Mieszanie
    float grassFactor = (3 - heightNorm) * slope;
    float rockFactor  = (1.0 - slope) * heightNorm;
    float snowFactor  = heightNorm * slope;

    // Unikamy dzielnia przez 0
    float sum = grassFactor + rockFactor + snowFactor;
    grassFactor /= sum;
    rockFactor  /= sum;
    snowFactor  /= sum;

    // Finalny kolor
    vec3 finalColor = 
          grass * grassFactor +
          rock  * rockFactor  +
          snow  * snowFactor;

    FragColor = vec4(finalColor, 1.0);
}
