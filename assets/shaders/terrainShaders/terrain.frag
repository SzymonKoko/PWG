#version 450 core

#engine_include "../common/pbr.glsl"

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;
in vec3 f_Normal;
in float f_WaterDepth;
in vec2 f_WorldUV;

uniform sampler2D u_debugTexture;
uniform sampler2D u_Watermap;
uniform sampler2D u_Depositmap;
uniform sampler2D u_FlowAccumulationmap;
uniform sampler2D u_RiverChannelmap;
uniform sampler2D u_SceneColor;
uniform int u_debugMode;
uniform bool u_debugEnabled;
uniform int u_worldSize;
uniform bool u_renderWaterOverlay;
uniform bool u_useWaterRefraction;
uniform vec2 u_viewportSize;

uniform sampler2DArray u_TexturesAlbedo;
uniform sampler2DArray u_TexturesNormal;
uniform sampler2DArray u_TexturesData;
uniform vec3 u_cameraPosition;
uniform float u_normalStrength;
uniform float u_snowHeight;
uniform float u_snowSoftness;
uniform float u_wetnessStrength;
uniform float u_waterAlpha;

// =========================
// HEIGHT SAMPLING (BLEND)
// =========================
float SampleHeight(vec2 uv, float rock)
{
    float gH = texture(u_TexturesData, vec3(uv, 9)).r;
    float rH = texture(u_TexturesData, vec3(uv, 10)).r;
    float sH = texture(u_TexturesData, vec3(uv, 11)).r;
    return mix(mix(gH, rH, rock), sH, 0.25);
}

// =========================
// PARALLAX OCCLUSION MAPPING
// =========================
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDirTS, float heightScale, float rock)
{
    float minLayers = 8.0;
    float maxLayers = 32.0;

    float numLayers = mix(maxLayers, minLayers, abs(viewDirTS.z));
    float layerDepth = 1.0 / numLayers;

    float currentLayerDepth = 0.0;

    vec2 P = viewDirTS.xy * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;

    float currentDepthMapValue = SampleHeight(currentTexCoords, rock);

    // marching
    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = SampleHeight(currentTexCoords, rock);
        currentLayerDepth += layerDepth;
    }

    // ===== refinement (interpolation)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = SampleHeight(prevTexCoords, rock) - (currentLayerDepth - layerDepth);

    float weight = afterDepth / (afterDepth - beforeDepth);

    return mix(currentTexCoords, prevTexCoords, weight);
}

// =========================
// MAIN
// =========================
void main()
{
    vec3 normal = normalize(f_Normal);

    // world UV (tiling)
    vec2 uv = f_Pos.xz * 0.01;

    float halfSize = (u_worldSize - 1.0) * 0.5;
    vec2 debugUV = (f_Pos.xz + halfSize) / u_worldSize;
    float waterDepth = f_WaterDepth;
    if (u_renderWaterOverlay && waterDepth < 0.012)
    {
        discard;
    }

    // ===== slope
    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

    float flowAccumulation = texture(u_FlowAccumulationmap, debugUV).r;
    float riverChannel = texture(u_RiverChannelmap, debugUV).r;

    float cliffRock = smoothstep(0.12, 0.82, slope);
    float exposedHighRock = smoothstep(u_snowHeight * 0.35, u_snowHeight * 0.95, f_Pos.y) * 0.35;
    float channelRock = smoothstep(0.18, 0.7, riverChannel) * smoothstep(0.08, 0.45, slope) * 0.35;
    float rock = clamp(max(cliffRock, exposedHighRock) + channelRock, 0.0, 1.0);

    // ===== TBN + view dir
    mat3 TBN = ComputeTBN(normal, uv, f_Pos);
    vec3 V = normalize(u_cameraPosition - f_Pos);
    vec3 viewDirTS = normalize(transpose(TBN) * V);

    // ===== distance fade
    float dist = length(u_cameraPosition - f_Pos);
    float fade = clamp(1.0 - dist / 50.0, 0.0, 1.0);

    // ===== PARALLAX
    float heightScale = mix(0.018, 0.045, rock);
    if (!u_renderWaterOverlay && fade > 0.01)
    {
        vec2 parallaxUV = ParallaxMapping(uv, viewDirTS, heightScale, rock);
        uv = mix(uv, parallaxUV, fade);
    }

    // =========================
    // GRASS
    // =========================
    vec3 gA = texture(u_TexturesAlbedo, vec3(uv, 0)).rgb;
    vec3 gN = texture(u_TexturesNormal, vec3(uv, 0)).rgb * 2.0 - 1.0;
    float gR = texture(u_TexturesData, vec3(uv, 0)).r;
    float gAO = texture(u_TexturesData, vec3(uv, 6)).r;

    // =========================
    // ROCK
    // =========================
    vec3 rA = texture(u_TexturesAlbedo, vec3(uv, 1)).rgb;
    vec3 rN = texture(u_TexturesNormal, vec3(uv, 1)).rgb * 2.0 - 1.0;
    float rR = texture(u_TexturesData, vec3(uv, 1)).r;
    float rAO = texture(u_TexturesData, vec3(uv, 7)).r;

    vec3 sA = texture(u_TexturesAlbedo, vec3(uv, 2)).rgb;
    vec3 sN = texture(u_TexturesNormal, vec3(uv, 2)).rgb * 2.0 - 1.0;
    float sR = texture(u_TexturesData, vec3(uv, 2)).r;
    float sAO = texture(u_TexturesData, vec3(uv, 8)).r;

    // =========================
    // BLEND
    // =========================
    vec3 albedo = mix(gA, rA, rock);
    vec3 normalTex = normalize(mix(gN, rN, rock));
    float roughness = mix(gR, rR, rock);
    float metallic = 0.0;
    float ao = mix(gAO, rAO, rock);

    float depositMask = texture(u_Depositmap, debugUV).r;
    float alluvium = smoothstep(0.006, 0.12, depositMask) * (1.0 - smoothstep(0.55, 0.95, rock));
    albedo = mix(albedo, sA, alluvium * 0.88);
    normalTex = normalize(mix(normalTex, sN, alluvium * 0.75));
    roughness = mix(roughness, sR, alluvium * 0.85);
    ao = mix(ao, sAO, alluvium * 0.65);

    float standingWater = smoothstep(0.008, 0.09, waterDepth);
    float flowWetness = max(smoothstep(0.08, 0.55, flowAccumulation), smoothstep(0.12, 0.65, riverChannel)) * 0.38;
    float wetness = clamp(max(standingWater, flowWetness) * u_wetnessStrength * (1.0 - rock * 0.35), 0.0, 1.0);
    albedo = mix(albedo, albedo * vec3(0.42, 0.48, 0.50), wetness);
    roughness = mix(roughness, 0.42, wetness * 0.75);

    float snowAltitude = smoothstep(u_snowHeight, u_snowHeight + max(u_snowSoftness, 1.0), f_Pos.y);
    float snowSlope = 1.0 - smoothstep(0.42, 0.88, slope);
    float snowMask = clamp(snowAltitude * snowSlope * (1.0 - wetness * 0.65) * (1.0 - alluvium * 0.75), 0.0, 1.0);
    vec3 snowColor = vec3(0.88, 0.90, 0.86);
    albedo = mix(albedo, snowColor, snowMask);
    roughness = mix(roughness, 0.72, snowMask);
    metallic = mix(metallic, 0.0, snowMask);
    ao = mix(ao, 0.92, snowMask * 0.65);

    // ===== FINAL NORMAL
    normalTex.xy *= u_normalStrength;
    normalTex = normalize(normalTex);
    vec3 detailNormal = normalize(TBN * normalTex);
    float detailStrength = clamp((1.0 - snowMask * 0.55) * (1.0 - wetness * 0.25), 0.0, 1.0);
    vec3 N = normalize(mix(normal, detailNormal, detailStrength));

    // ===== LIGHTING
    vec3 litColor = ApplyLighting(N, V, albedo, metallic, roughness, ao, f_Pos);

    float visibleWater = smoothstep(0.025, 0.16, waterDepth);
    float riverTrace = max(smoothstep(0.08, 0.45, flowAccumulation), smoothstep(0.12, 0.6, riverChannel)) * smoothstep(0.01, 0.08, waterDepth);
    float deepWater = smoothstep(0.16, 0.55, waterDepth);
    vec3 shallowWaterColor = vec3(0.05, 0.48, 0.72);
    vec3 deepWaterColor = vec3(0.0, 0.08, 0.22);
    vec3 riverWaterColor = vec3(0.0, 0.62, 0.82);
    vec3 waterColor = mix(shallowWaterColor, deepWaterColor, deepWater);
    waterColor = mix(waterColor, riverWaterColor, riverTrace * 0.6);
    float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);

    if (u_renderWaterOverlay)
    {
        vec2 texelSize = 1.0 / vec2(textureSize(u_Watermap, 0));
        float wL = texture(u_Watermap, f_WorldUV + vec2(-texelSize.x, 0.0)).r;
        float wR = texture(u_Watermap, f_WorldUV + vec2( texelSize.x, 0.0)).r;
        float wD = texture(u_Watermap, f_WorldUV + vec2(0.0, -texelSize.y)).r;
        float wU = texture(u_Watermap, f_WorldUV + vec2(0.0,  texelSize.y)).r;
        vec2 refractionOffset = vec2(wL - wR, wD - wU) * 0.12;
        refractionOffset += vec2(flowAccumulation - riverChannel, riverChannel - flowAccumulation) * 0.004;

        vec3 refractedScene;
        if (u_useWaterRefraction)
        {
            vec2 screenUV = gl_FragCoord.xy / max(u_viewportSize, vec2(1.0));
            refractedScene = texture(u_SceneColor, clamp(screenUV + refractionOffset, vec2(0.0), vec2(1.0))).rgb;
        }
        else
        {
            vec3 refractedGrass = texture(u_TexturesAlbedo, vec3(uv + refractionOffset, 0)).rgb;
            vec3 refractedRock = texture(u_TexturesAlbedo, vec3(uv + refractionOffset, 1)).rgb;
            vec3 refractedSediment = texture(u_TexturesAlbedo, vec3(uv + refractionOffset, 2)).rgb;
            refractedScene = mix(mix(refractedGrass, refractedRock, rock), refractedSediment, alluvium);
        }
        float foam = smoothstep(0.22, 0.85, riverChannel + flowAccumulation) * smoothstep(0.02, 0.12, waterDepth) * (1.0 - deepWater);
        vec3 transparentWater = mix(refractedScene, waterColor, 0.26 + deepWater * 0.34 + riverTrace * 0.12);
        transparentWater = mix(transparentWater, vec3(0.82, 0.95, 0.96), foam * 0.35);
        transparentWater += fresnel * 0.28;

        float alpha = clamp((0.16 + visibleWater * 0.24 + deepWater * 0.26 + riverTrace * 0.12) * u_waterAlpha, 0.08, 0.82);
        FragColor = vec4(transparentWater, alpha);
        return;
    }


    // =========================
    // DEBUG
    // =========================
    if (u_debugEnabled)
    {
        if (u_debugMode == 1) // HEIGHT
        {
            float h = texture(u_debugTexture, debugUV).r;
            h = log(h + 1.0);
            h = clamp(h, 0.0, 1.0);
            FragColor = vec4(vec3(h), 1.0);
            return;
        }
        else if (u_debugMode == 2) // NORMAL
        {
            FragColor = vec4(normal * 0.5 + 0.5, 1.0);
            return;
        }
        else if (u_debugMode == 3) // SLOPE
        {
            FragColor = vec4(vec3(slope), 1.0);
            return;
        }
        else if (u_debugMode == 4) // WATER
        {
            float water = texture(u_debugTexture, debugUV).r;
            water = clamp(water * 8.0, 0.0, 1.0);
            FragColor = vec4(mix(vec3(0.02), vec3(0.0, 0.35, 1.0), water), 1.0);
            return;
        }
        else if (u_debugMode == 5) // SEDIMENT
        {
            float sediment = texture(u_debugTexture, debugUV).r;
            sediment = clamp(sediment * 20.0, 0.0, 1.0);
            FragColor = vec4(mix(vec3(0.05), vec3(0.85, 0.55, 0.2), sediment), 1.0);
            return;
        }
        else if (u_debugMode == 6) // WATER FLUX
        {
            vec4 flux = texture(u_debugTexture, debugUV);
            float leftRight = flux.y - flux.x;
            float upDown = flux.w - flux.z;
            float strength = clamp((flux.x + flux.y + flux.z + flux.w) * 12.0, 0.0, 1.0);
            vec3 directionColor = vec3(leftRight * 0.5 + 0.5, upDown * 0.5 + 0.5, 0.15);
            FragColor = vec4(mix(vec3(0.02), directionColor, strength), 1.0);
            return;
        }
        else if (u_debugMode == 7) // DIAGONAL FLUX
        {
            vec4 flux = texture(u_debugTexture, debugUV);
            float leftRight = (flux.y + flux.w) - (flux.x + flux.z);
            float upDown = (flux.z + flux.w) - (flux.x + flux.y);
            float strength = clamp((flux.x + flux.y + flux.z + flux.w) * 12.0, 0.0, 1.0);
            vec3 directionColor = vec3(leftRight * 0.5 + 0.5, upDown * 0.5 + 0.5, 0.35);
            FragColor = vec4(mix(vec3(0.02), directionColor, strength), 1.0);
            return;
        }
        else if (u_debugMode == 8) // EROSION DELTA
        {
            float delta = texture(u_debugTexture, debugUV).r;
            float erosion = clamp(-delta * 200.0, 0.0, 1.0);
            float deposition = clamp(delta * 200.0, 0.0, 1.0);
            vec3 erosionColor = vec3(0.9, 0.1, 0.05);
            vec3 depositionColor = vec3(0.05, 0.7, 0.15);
            vec3 c = mix(vec3(0.02), erosionColor, erosion);
            c = mix(c, depositionColor, deposition);
            FragColor = vec4(c, 1.0);
            return;
        }
        else if (u_debugMode == 9) // FLOW ACCUMULATION
        {
            float flowAccumulation = texture(u_debugTexture, debugUV).r;
            vec3 c = mix(vec3(0.02), vec3(0.95, 0.8, 0.15), clamp(flowAccumulation, 0.0, 1.0));
            FragColor = vec4(c, 1.0);
            return;
        }
        else if (u_debugMode == 10) // RIVER CHANNELS
        {
            float channel = texture(u_debugTexture, debugUV).r;
            vec3 c = mix(vec3(0.02), vec3(0.0, 0.7, 0.55), clamp(channel, 0.0, 1.0));
            FragColor = vec4(c, 1.0);
            return;
        }
        else if (u_debugMode == 11) // RIVERS / LAKES
        {
            float water = texture(u_debugTexture, debugUV).r;
            float flowAccumulation = texture(u_FlowAccumulationmap, debugUV).r;
            float channel = texture(u_RiverChannelmap, debugUV).r;
            float river = max(smoothstep(0.04, 0.4, flowAccumulation), smoothstep(0.12, 0.6, channel)) * smoothstep(0.01, 0.12, water);
            float lake = smoothstep(0.14, 0.55, water) * (1.0 - smoothstep(0.35, 0.8, max(flowAccumulation, channel)));
            vec3 riverColor = vec3(0.0, 0.3, 1.0);
            vec3 lakeColor = vec3(0.0, 0.08, 0.45);
            vec3 c = mix(litColor, riverColor, river);
            c = mix(c, lakeColor, lake);

            FragColor = vec4(c, 1.0);

            return;
        }
        else if (u_debugMode == 12) // DEPOSITED SEDIMENT
        {
            float deposit = texture(u_debugTexture, debugUV).r;
            deposit = clamp(deposit * 14.0, 0.0, 1.0);
            FragColor = vec4(mix(vec3(0.04), vec3(0.8, 0.58, 0.32), deposit), 1.0);
            return;
        }
        
        

    }

    FragColor = vec4(litColor, 1.0);
}
