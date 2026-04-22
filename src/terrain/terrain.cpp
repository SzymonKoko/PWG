#include "terrain.h"

namespace pwg
{

    Terrain::Terrain(std::shared_ptr<ResourceManager> resourceManager)
    {

        m_material = resourceManager->GetMaterialManager()->GetMaterial("TerrainMaterial");
        m_terrainLayersManager = std::make_unique<TerrainLayersManager>();
        m_chunkManager = std::make_shared<ChunkManager>(resourceManager, m_terrainSettings, m_material);

        m_size = CHUNKS * m_terrainSettings.chunkSize;

    }

    Terrain::~Terrain()
    {
    }

    void Terrain::Update(float dt, std::shared_ptr<ICamera> camera)
    {
        bool regenerateLayers = false;

        for (auto& layer : m_terrainLayersManager->GetLayers())
        {
            if (layer.dirty)
            {
                regenerateLayers = true;
            }
        }

        m_chunkManager->Update(camera);


    }

    void Terrain::Draw(Renderer& renderer)
    {
        m_chunkManager->Draw();
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
        return m_size;
    }

    TerrainNoiseSettings& Terrain::GetNoiseSettings()
    {
        return m_terrainNoiseSettings;
    }

    std::vector<TerrainLayer>& Terrain::GetTerrainLayers()
    {
        return m_terrainLayersManager->GetLayers();
    }

    void Terrain::SetTerrainTextures(std::unordered_map<std::string, std::shared_ptr<Texture>>& textures)
    {
        m_terrainTextures.heightmap = textures.at("FinalHeight");
        m_terrainTextures.normalmap = textures.at("NormalMask");
    }

    float Terrain::GetTerrainHeightAt(int x, int z)
    {
        //auto heightmapSample = m_terrainGenerator->GetCPUHeightmap();
        x = (x - 0.0) / m_size;
        z = (z - 0.0) / m_size;
        return 0;// heightmapSample[z * m_size + x] * m_terrainNoiseSettings.amplitude;
    }
}
