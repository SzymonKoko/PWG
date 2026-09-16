#version 450 core

in float f_Sediment;
out vec4 FragColor;

void main()
{
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float radius = dot(p, p);
    if (radius > 1.0) discard;
    vec3 clearWater = vec3(0.24, 0.68, 0.92);
    vec3 sedimentWater = vec3(0.43, 0.32, 0.18);
    FragColor = vec4(mix(clearWater, sedimentWater, f_Sediment), (1.0 - radius) * 0.78);
}
