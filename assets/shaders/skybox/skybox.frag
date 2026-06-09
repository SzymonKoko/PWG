#version 450 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube u_skybox;
uniform int u_useSkybox;
uniform vec3 u_sunDirection;
uniform vec3 u_sunColor;
uniform float u_sunIntensity;
uniform float u_sunAngularSize;
uniform vec3 u_skyTint;
uniform vec3 u_groundAlbedo;
uniform float u_atmosphereDensity;
uniform float u_haze;
uniform float u_skyboxBlend;

void main()
{
    vec3 direction = normalize(TexCoords);
    vec3 toSun = normalize(-u_sunDirection);
    float up = clamp(direction.y * 0.5 + 0.5, 0.0, 1.0);
    float horizon = pow(1.0 - abs(direction.y), 2.0);
    float sunHeight = clamp(toSun.y, 0.0, 1.0);

    vec3 zenith = u_skyTint * mix(0.18, 0.95, sunHeight);
    vec3 horizonColor = mix(vec3(0.95, 0.42, 0.18), vec3(0.72, 0.82, 0.95), sunHeight);
    vec3 ground = u_groundAlbedo * 0.22;
    vec3 sky = mix(ground, mix(horizonColor, zenith, pow(up, 0.65)), smoothstep(0.02, 0.18, up));

    float sunCos = dot(direction, toSun);
    float sunRadius = radians(max(u_sunAngularSize, 0.01)) * 0.5;
    float sunDisk = smoothstep(cos(sunRadius * 1.15), cos(sunRadius * 0.75), sunCos);
    float solarGlow = pow(max(sunCos, 0.0), mix(32.0, 6.0, clamp(u_haze, 0.0, 1.0))) * (0.25 + u_haze);
    float atmosphere = max(u_atmosphereDensity, 0.01);

    sky += u_sunColor * solarGlow * mix(0.12, 0.55, sunHeight) * atmosphere;
    sky += u_sunColor * sunDisk * max(u_sunIntensity, 0.0) * 1.8;
    sky += horizonColor * horizon * u_haze * 0.18;

    if (u_useSkybox != 0)
    {
        vec3 cubemap = texture(u_skybox, direction).rgb;
        sky = mix(sky, cubemap, clamp(u_skyboxBlend, 0.0, 1.0));
    }

    FragColor = vec4(max(sky, vec3(0.0)), 1.0);
}
