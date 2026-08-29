#version 450 core

out vec4 FragColor;

in vec3 f_WorldPos;
in vec3 f_MacroNormal;
in vec2 f_WorldUV;
in float f_WaterDepth;
in float f_Flow;
in float f_Channel;

uniform sampler2D u_SceneColor;
uniform sampler2D u_SceneDepth;
uniform sampler2D u_WaterNormalLarge;
uniform sampler2D u_WaterNormalDetail;
uniform vec2 u_viewportSize;
uniform mat4 u_inverseProjection;
uniform vec3 u_cameraPosition;
uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform float u_lightIntensity;
uniform vec3 u_skyTint;
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

void main()
{
    float visibilityFeather = max(0.012, u_minVisibleWaterDepth * 0.75);
    float accumulatedWater = smoothstep(u_minVisibleWaterDepth, u_minVisibleWaterDepth + visibilityFeather, f_WaterDepth);
    float activeFlow = smoothstep(0.10, 0.32, max(f_Flow, f_Channel));
    float waterCoverage = max(accumulatedWater, activeFlow);
    if (waterCoverage < 0.01)
    {
        discard;
    }

    vec2 viewport = max(u_viewportSize, vec2(1.0));
    vec2 screenUV = gl_FragCoord.xy / viewport;
    float speed = u_time * 0.018 * u_waterWaveSpeed;
    float flowStrength = smoothstep(0.12, 0.72, max(f_Flow, f_Channel));
    float lakeCalm = 1.0 - flowStrength * 0.55;

    vec2 largeUV = f_WorldPos.xz * (0.018 * u_waterWaveScale) + vec2(speed, speed * 0.37);
    vec2 detailUV = f_WorldPos.xz * (0.067 * u_waterWaveScale) + vec2(-speed * 0.71, speed * 1.13);
    vec3 largeNormal = DecodeNormal(texture(u_WaterNormalLarge, largeUV).xyz);
    vec3 detailNormal = DecodeNormal(texture(u_WaterNormalDetail, detailUV).xyz);
    vec2 waveSlope = (largeNormal.xy * 0.68 + detailNormal.xy * 0.32) * u_waterNormalStrength * lakeCalm;
    vec3 waveNormal = normalize(vec3(waveSlope.x, 1.0, waveSlope.y));
    vec3 N = normalize(mix(normalize(f_MacroNormal), waveNormal, 0.82));
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
    vec3 skyReflection = u_skyTint * mix(0.72, 1.35, max(N.y, 0.0));
    float roughness = clamp(u_waterRoughness, 0.04, 0.5);
    float sunPower = mix(900.0, 48.0, roughness);
    float sunSpecular = pow(max(dot(N, H), 0.0), sunPower) * u_lightIntensity;
    vec3 reflected = skyReflection + u_lightColor * sunSpecular;
    vec3 color = mix(transmitted, reflected, clamp(fresnel * 0.72, 0.0, 0.88));

    float shoreFoam = 1.0 - smoothstep(0.04, max(u_waterFoamWidth, 0.05), opticalThickness);
    shoreFoam *= smoothstep(0.004, 0.035, f_WaterDepth);
    float flowFoam = smoothstep(0.42, 0.92, f_Flow + f_Channel * 0.7) * (1.0 - depthColor * 0.7);
    float foam = clamp(max(shoreFoam, flowFoam * 0.55) * u_waterFoamIntensity, 0.0, 0.8);
    color = mix(color, vec3(0.82, 0.93, 0.94), foam);

    float cameraDistance = length(u_cameraPosition - f_WorldPos);
    float fog = 1.0 - exp(-cameraDistance * max(u_fogDensity, 0.0));
    color = mix(color, u_skyTint, clamp(fog, 0.0, 0.82));
    color = mix(refractedScene, color, waterCoverage);
    FragColor = vec4(max(color, vec3(0.0)), 1.0);
}
