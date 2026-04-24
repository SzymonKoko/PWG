#version 450 core

#engine_include "../common/pbr.glsl"

out vec4 FragColor;

in vec2 f_TextureCoordinates;
in vec3 f_Pos;
in vec3 f_Normal;

uniform sampler2D u_debugTexture;
uniform int u_debugMode;
uniform bool u_debugEnabled;
uniform int u_worldSize;

uniform sampler2DArray u_Textures;
uniform vec3 u_cameraPosition;

// =========================
// HEIGHT SAMPLING (BLEND)
// =========================
float SampleHeight(vec2 uv, float rock)
{
    float gH = texture(u_Textures, vec3(uv, 5)).r;
    float rH = texture(u_Textures, vec3(uv, 10)).r;
    return mix(gH, rH, rock);
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

    // ===== slope
    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

    // ===== material blend
    float rock = smoothstep(0.05, 0.95, slope);

    // ===== TBN + view dir
    mat3 TBN = ComputeTBN(normal, uv, f_Pos);
    vec3 V = normalize(u_cameraPosition - f_Pos);
    vec3 viewDirTS = normalize(transpose(TBN) * V);

    // ===== distance fade
    float dist = length(u_cameraPosition - f_Pos);
    float fade = clamp(1.0 - dist / 50.0, 0.0, 1.0);

    // ===== PARALLAX
    float heightScale = 0.04; // 🔥 tweak 0.02–0.06
    vec2 parallaxUV = ParallaxMapping(uv, viewDirTS, heightScale, rock);

    uv = mix(uv, parallaxUV, fade);

    // =========================
    // GRASS
    // =========================
    vec3 gA = texture(u_Textures, vec3(uv, 0)).rgb;
    vec3 gN = texture(u_Textures, vec3(uv, 1)).rgb * 2.0 - 1.0;
    float gR = texture(u_Textures, vec3(uv, 2)).r;
    float gM = texture(u_Textures, vec3(uv, 3)).r;
    float gAO = texture(u_Textures, vec3(uv, 4)).r;

    // =========================
    // ROCK
    // =========================
    vec3 rA = texture(u_Textures, vec3(uv, 5)).rgb;
    vec3 rN = texture(u_Textures, vec3(uv, 6)).rgb * 2.0 - 1.0;
    float rR = texture(u_Textures, vec3(uv, 7)).r;
    float rM = texture(u_Textures, vec3(uv, 8)).r;
    float rAO = texture(u_Textures, vec3(uv, 9)).r;

    // =========================
    // BLEND
    // =========================
    vec3 albedo = mix(gA, rA, rock);
    vec3 normalTex = normalize(mix(gN, rN, rock));
    float roughness = mix(gR, rR, rock);
    float metallic = mix(gM, rM, rock);
    float ao = mix(gAO, rAO, rock);

    // ===== FINAL NORMAL
    vec3 N = normal;

    // ===== LIGHTING
    vec3 litColor = ApplyLighting(N, V, albedo, metallic, roughness, ao, f_Pos);

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
    }

    FragColor = vec4(litColor, 1.0);
}