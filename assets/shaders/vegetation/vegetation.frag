#version 450 core

in vec3 f_WorldPos;
in vec3 f_Normal;
in vec3 f_Color;
in vec2 f_Uv;

out vec4 FragColor;

uniform sampler2D u_albedo;
uniform int u_hasAlbedo;
uniform int u_isGrass;
uniform int u_isTree;
uniform vec3 u_baseColor;
uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform float u_lightIntensity;
uniform vec3 u_cameraPosition;
uniform float u_renderDistance;
uniform float u_skyAmbient;
uniform vec3 u_skyTint;
uniform sampler2D u_Watermap;
uniform sampler2D u_FlowAccumulationmap;
uniform sampler2D u_RiverChannelmap;
uniform float u_waterWorldSize;
uniform float u_minVisibleWaterDepth;

void main()
{
	if (u_isGrass == 1)
	{
		vec2 waterUV = f_WorldPos.xz / max(u_waterWorldSize, 1.0) + vec2(0.5);
		float waterDepth = texture(u_Watermap, waterUV).r;
		float depthCoverage = smoothstep(u_minVisibleWaterDepth, u_minVisibleWaterDepth + max(0.012, u_minVisibleWaterDepth * 0.75), waterDepth);
		if (depthCoverage > 0.05)
		{
			discard;
		}
	}

    vec4 sampled = u_hasAlbedo == 1 ? texture(u_albedo, f_Uv) : vec4(1.0);
    if (sampled.a < 0.35)
    {
        discard;
    }
    if (u_isGrass == 1)
    {
        float distanceToCamera = distance(f_WorldPos.xz, u_cameraPosition.xz);
        float fadeStart = u_renderDistance * 0.77;
        if (distanceToCamera > u_renderDistance || sampled.a * (1.0 - smoothstep(fadeStart, u_renderDistance, distanceToCamera)) < 0.35)
        {
            discard;
        }
    }

    vec3 normal = normalize(f_Normal);
    if (!gl_FrontFacing) normal = -normal;
    float sunFacing = dot(normal, normalize(-u_lightDirection));
    float diffuseAmount = u_isGrass == 1
        ? 0.32 + 0.68 * abs(sunFacing)
        : max(sunFacing, 0.0);
    vec3 albedo = sampled.rgb * f_Color * u_baseColor;
    if (u_isGrass == 1)
    {
        albedo = pow(max(albedo, vec3(0.0)), vec3(0.82)) * 1.12;
    }
    float daylight = clamp(u_lightIntensity / 7.5, 0.0, 1.25);
    float sunlight = u_lightIntensity * 0.16;
    vec3 ambient = albedo * (0.16 + u_skyAmbient * (0.45 + daylight * 0.75)) * mix(vec3(1.0), u_skyTint, 0.18);
    vec3 diffuse = albedo * u_lightColor * diffuseAmount * sunlight;
    FragColor = vec4(ambient + diffuse, 1.0);
}
