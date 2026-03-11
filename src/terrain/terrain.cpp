#include "terrain.h"

namespace pwg
{

    Terrain::Terrain(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
        : m_mesh(mesh),
        m_material(material)
    {
        m_size = 2000;

        m_terrainLayersManager = std::make_unique<TerrainLayersManager>();

        auto materialProps = m_material->GetMaterialProperties();

        materialProps.ambient = glm::vec3(0.2f);//{ 1.0f, 0.5f, 0.31f };
        materialProps.diffuse = glm::vec3(1.0f);//{ 1.0f, 0.5f, 0.31f };
        materialProps.specular = { 0.5f, 0.5f, 0.5f };
        materialProps.shininess = 12.0f;

        m_material->SetMaterialProperties(materialProps);

    }

    Terrain::~Terrain()
    {
    }

    void Terrain::Update(float dt)
    {
        bool regenerateLayers = false;

        for (auto& layer : m_terrainLayersManager->GetLayers())
        {
            if (layer.dirty)
            {
                regenerateLayers = true;
            }
        }

        m_mesh->Update();


    }

    void Terrain::Draw(Renderer& renderer)
    {

        m_material->Apply();

        auto modelMatrix = m_mesh->GetModelMatrix();

        m_material->SetUniformMat4("u_model", modelMatrix);

        m_material->SetUniformFloat("u_amplitude", m_terrainNoiseSettings.amplitude);



        m_material->SetUniformTexture("u_Heightmap", m_terrainTextures.heightmap, 0);


        if (m_terrainTextures.normalmap)
        {
            m_material->SetUniformTexture("u_Normalmap", m_terrainTextures.normalmap, 1);
        }
         /*
         if (m_terrainTextures && m_terrainTextures->splatmap)
         {
             m_material->SetUniformTexture("u_Splatmap", m_terrainTextures->splatmap, 2);
         }*/

        m_mesh->Draw();
    }

    std::shared_ptr<Mesh> Terrain::GetMesh()
    {
        return m_mesh;
    }

    std::shared_ptr<Material> Terrain::GetMaterial()
    {
        return m_material;
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
