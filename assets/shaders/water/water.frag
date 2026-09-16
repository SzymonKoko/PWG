#version 450 core

out vec4 FragColor;

in vec3 f_WorldPos;
in vec3 f_MacroNormal;
in vec2 f_WorldUV;
in float f_RenderLod;

uniform sampler2D u_SceneColor;
uniform sampler2D u_SceneDepth;
uniform sampler2D u_WaterNormalLarge;
uniform sampler2D u_WaterNormalDetail;
uniform sampler2D u_WaterCoverageCache;
uniform sampler2D u_WaterSurfaceCache;
uniform sampler2D u_Watermap;
uniform sampler2D u_FlowAccumulationmap;
uniform sampler2D u_RiverChannelmap;
uniform sampler2D u_WaterVelocityMap;
uniform samplerCube u_skybox;
uniform vec2 u_viewportSize;
uniform mat4 u_inverseProjection;
uniform vec3 u_cameraPosition;
uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform float u_lightIntensity;
uniform vec3 u_skyTint;
uniform int u_useSkybox;
uniform float u_skyboxBlend;
uniform float u_fogDensity;
uniform float u_time;
uniform bool u_useWaterRefraction;
uniform float u_minVisibleWaterDepth;

uniform float u_waterAlpha;
uniform vec3 u_waterShallowColor;
uniform vec3 u_waterDeepColor;
uniform float u_waterNormalStrength;
uniform float u_waterWaveScale;
uniform float u_waterWaveSpeed;
uniform float u_waterRefractionStrength;
uniform float u_waterAbsorption;
uniform float u_waterFoamWidth;
uniform float u_waterFoamIntensity;
uniform float u_waterRoughness;

float ViewDepth(float depth, vec2 screenUV)
{
    vec4 clip = vec4(screenUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = u_inverseProjection * clip;
    return max(-view.z / max(view.w, 0.00001), 0.0);
}

vec3 DecodeNormal(vec3 encoded)
{
    return normalize(encoded * 2.0 - 1.0);
}

float HashNoise(vec2 point)
{
    point = fract(point * vec2(123.34, 456.21));
    point += dot(point, point + 45.32);
    return fract(point.x * point.y);
}

float SmoothNoise(vec2 point)
{
    vec2 cell = floor(point);
    vec2 local = fract(point);
    local = local * local * (3.0 - 2.0 * local);
    return mix(
        mix(HashNoise(cell), HashNoise(cell + vec2(1.0, 0.0)), local.x),
        mix(HashNoise(cell + vec2(0.0, 1.0)), HashNoise(cell + vec2(1.0)), local.x),
        local.y);
}

void main()
{
    if (any(lessThan(f_WorldUV, vec2(0.0))) || any(greaterThan(f_WorldUV, vec2(1.0)))) discard;
    float f_WaterDepth = max(texture(u_Watermap, f_WorldUV).r, 0.0);
    float f_Flow = max(texture(u_FlowAccumulationmap, f_WorldUV).r, 0.0);
    float f_Channel = max(texture(u_RiverChannelmap, f_WorldUV).r, 0.0);
    float cachedCoverage = textureLod(u_WaterCoverageCache, f_WorldUV, f_RenderLod).r;
    float cameraDistance = length(u_cameraPosition - f_WorldPos);
    float visibilityFeather = max(0.012, u_minVisibleWaterDepth * 0.75);
    float accumulatedWater = smoothstep(u_minVisibleWaterDepth, u_minVisibleWaterDepth + visibilityFeather, f_WaterDepth);
    float thinWater = smoothstep(max(0.0005, u_minVisibleWaterDepth * 0.05), max(0.002, u_minVisibleWaterDepth * 0.40), f_WaterDepth);
    float activeFlow = thinWater * smoothstep(0.10, 0.32, max(f_Flow, f_Channel));
    // Coarse coverage keeps narrow channels in the clipmap geometry, but it must
    // never turn a dry full-resolution sample into visible water.
    float waterCoverage = max(accumulatedWater, activeFlow) * smoothstep(0.001, 0.08, cachedCoverage);
    if (waterCoverage < 0.01)
    {
        discard;
    }

    vec2 viewport = max(u_viewportSize, vec2(1.0));
    vec2 screenUV = gl_FragCoord.xy / viewport;
    float speed = u_time * 0.018 * u_waterWaveSpeed;
    float flowStrength = smoothstep(0.12, 0.72, max(f_Flow, f_Channel));
    float lakeCalm = 1.0 - flowStrength * 0.55;

	vec2 generatedVelocity = texture(u_WaterVelocityMap, f_WorldUV).rg;
	float velocityMagnitude = length(generatedVelocity);
	vec2 downhillDirection = velocityMagnitude > 0.000001 ? generatedVelocity / velocityMagnitude : vec2(0.0);
	float directionalFlow = smoothstep(0.02, 0.35, velocityMagnitude) * flowStrength;

    // A positive texture offset moves the visible pattern in the opposite world
    // direction, hence the minus sign for downhill advection.
    vec2 windOffsetLarge = vec2(speed, speed * 0.37);
    vec2 windOffsetDetail = vec2(-speed * 0.71, speed * 1.13);
    vec2 flowOffset = -downhillDirection * speed * 1.35;
    vec2 largeUV = f_WorldPos.xz * (0.018 * u_waterWaveScale)
        + mix(windOffsetLarge * 0.28, flowOffset, directionalFlow);
    vec2 detailUV = f_WorldPos.xz * (0.067 * u_waterWaveScale)
        + mix(windOffsetDetail * 0.24, flowOffset * 1.7, directionalFlow);
    float detailFade = 1.0 - smoothstep(450.0, 2200.0, cameraDistance);
	float standingWater = smoothstep(u_minVisibleWaterDepth + 0.015, u_minVisibleWaterDepth + 0.10, f_WaterDepth)
		* (1.0 - smoothstep(0.18, 0.65, max(f_Flow, f_Channel)));
	vec3 macroNormal = normalize(mix(normalize(f_MacroNormal), vec3(0.0, 1.0, 0.0), standingWater * 0.92));
    vec3 largeNormal = DecodeNormal(texture(u_WaterNormalLarge, largeUV).xyz);
    vec3 detailNormal = DecodeNormal(texture(u_WaterNormalDetail, detailUV).xyz);
    vec2 waveSlope = (largeNormal.xy * 0.68 + detailNormal.xy * 0.32 * detailFade)
        * u_waterNormalStrength * lakeCalm * mix(0.12, 1.0, detailFade);
    vec3 waveNormal = normalize(vec3(waveSlope.x, 1.0, waveSlope.y));
    vec3 N = normalize(mix(macroNormal, waveNormal, 0.82));
    vec3 V = normalize(u_cameraPosition - f_WorldPos);
    vec3 L = normalize(-u_lightDirection);
    vec3 H = normalize(V + L);

    float sceneDepth = ViewDepth(texture(u_SceneDepth, screenUV).r, screenUV);
    float waterDepth = ViewDepth(gl_FragCoord.z, screenUV);
    float opticalThickness = max(sceneDepth - waterDepth, 0.0);
    opticalThickness = max(opticalThickness, f_WaterDepth * 0.55);
    float absorption = 1.0 - exp(-opticalThickness * max(u_waterAbsorption, 0.001));

    vec2 refractionOffset = N.xz * (u_waterRefractionStrength / viewport) * mix(0.35, 1.0, 1.0 - absorption);
    refractionOffset *= u_useWaterRefraction ? 1.0 : 0.0;
    vec3 refractedScene = texture(u_SceneColor, clamp(screenUV + refractionOffset, vec2(0.002), vec2(0.998))).rgb;

    float depthColor = clamp(absorption + smoothstep(0.15, 1.4, f_WaterDepth) * 0.35, 0.0, 1.0);
    vec3 bodyColor = mix(u_waterShallowColor, u_waterDeepColor, depthColor);
    float tintStrength = clamp((0.14 + depthColor * 0.68) * u_waterAlpha, 0.0, 0.92);
    vec3 transmitted = mix(refractedScene, bodyColor, tintStrength);

    float fresnel = 0.02 + 0.98 * pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 5.0);
    float roughness = clamp(u_waterRoughness, 0.04, 0.5);
    vec3 reflectionDirection = reflect(-V, N);
    vec3 cubemapReflection = textureLod(u_skybox, reflectionDirection, roughness * 8.0).rgb;
    vec3 proceduralReflection = u_skyTint * mix(0.72, 1.35, max(N.y, 0.0));
    float cubemapWeight = float(u_useSkybox != 0) * clamp(u_skyboxBlend, 0.0, 1.0);
    vec3 skyReflection = mix(proceduralReflection, cubemapReflection, cubemapWeight);
    float sunPower = mix(900.0, 48.0, roughness);
    float sunSpecular = pow(max(dot(N, H), 0.0), sunPower) * u_lightIntensity;
    vec3 reflected = skyReflection + u_lightColor * sunSpecular;
    vec3 color = mix(transmitted, reflected, clamp(fresnel * 0.72, 0.0, 0.88));

    float shoreFoam = 1.0 - smoothstep(0.04, max(u_waterFoamWidth, 0.05), opticalThickness);
    shoreFoam *= smoothstep(0.004, 0.035, f_WaterDepth);
    float shoreBreakup = SmoothNoise(f_WorldPos.xz * 0.18 + vec2(u_time * 0.018, -u_time * 0.011));
    shoreBreakup = mix(0.48, 1.0, smoothstep(0.18, 0.82, shoreBreakup));
    shoreFoam *= shoreBreakup;
    float flowFoam = smoothstep(0.42, 0.92, f_Flow + f_Channel * 0.7) * (1.0 - depthColor * 0.7);
    float foam = clamp(max(shoreFoam, flowFoam * 0.55) * u_waterFoamIntensity * detailFade, 0.0, 0.8);
    color = mix(color, vec3(0.82, 0.93, 0.94), foam);

    float fog = 1.0 - exp(-cameraDistance * max(u_fogDensity, 0.0));
    color = mix(color, u_skyTint, clamp(fog, 0.0, 0.82));
    color = mix(refractedScene, color, waterCoverage);
    FragColor = vec4(max(color, vec3(0.0)), 1.0);
}
