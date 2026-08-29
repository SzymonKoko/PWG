#include "terrain.h"

namespace pwg
{

    Terrain::Terrain(std::shared_ptr<ResourceManager> resourceManager)
    {

        m_material = resourceManager->GetMaterialManager()->GetMaterial("TerrainMaterial");
        auto waterMaterial = resourceManager->GetMaterialManager()->GetMaterial("WaterMaterial");
        m_terrainDebug = std::make_shared<TerrainDebug>();
        m_chunkManager = std::make_shared<ChunkManager>(resourceManager, m_terrainSettings, m_material, waterMaterial, m_terrainDebug);

        m_size = m_chunkManager->GetWorldSize();

    }

    Terrain::~Terrain()
    {
    }

    void Terrain::Update(float dt, std::shared_ptr<ICamera> camera)
    {
        m_chunkManager->Update(camera, dt);
    }

    void Terrain::Draw(Renderer& renderer)
    {
        m_chunkManager->Draw();
    }

    void Terrain::DrawWater(Renderer& renderer)
    {
        m_chunkManager->DrawWater(renderer);
    }

    void Terrain::DrawShadowMap(std::shared_ptr<Shader> shader, const glm::mat4& lightSpaceMatrix)
    {
        m_chunkManager->DrawShadowMap(shader, lightSpaceMatrix);
    }

    std::shared_ptr<Mesh> Terrain::GetMesh()
    {
        return m_mesh;
    }

    std::shared_ptr<Material> Terrain::GetMaterial()
    {
        return m_material;
    }

    std::shared_ptr<ChunkManager> Terrain::GetChunkManager()
    {
        return m_chunkManager;
    }

    int Terrain::GetSize()
    {
        return m_chunkManager->GetWorldSize();
    }

    TerrainNoiseSettings& Terrain::GetNoiseSettings()
    {
        return m_terrainNoiseSettings;
    }

    void Terrain::SetTerrainTextures(std::unordered_map<std::string, std::shared_ptr<Texture>>& textures)
    {
        m_terrainTextures.heightmap = textures.at("FinalHeight");
        m_terrainTextures.normalmap = textures.at("NormalMask");
    }

    float Terrain::GetTerrainHeightAt(float x, float z)
    {
        return m_chunkManager->SampleHeightAtWorld(x, z);
    }
    std::shared_ptr<TerrainDebug> Terrain::GetTerrainDebug()
    {
        return m_terrainDebug;
    }
}
