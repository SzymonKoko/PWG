#include "terrain.h"

pwg::Terrain::Terrain(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, TerrainComputeShaders& computeShaders)
    : m_mesh(mesh),
      m_material(material),
      m_terrainComputeShaders(computeShaders)
{
    m_size = 2048;

    m_terrainGenerator = std::make_unique<TerrainGenerator>(m_terrainComputeShaders.heightmapShader, m_terrainComputeShaders.normalmapShader, m_terrainComputeShaders.splatmapShader);
    m_terrainLayersManager = std::make_unique<TerrainLayersManager>();
    m_terrainTextures = std::make_shared<TerrainTextures>();

    auto materialProps = m_material->GetMaterialProperties(); 

    materialProps.ambient = glm::vec3(0.2f);//{ 1.0f, 0.5f, 0.31f };
    materialProps.diffuse = glm::vec3(1.0f);//{ 1.0f, 0.5f, 0.31f };
    materialProps.specular = { 0.5f, 0.5f, 0.5f };
    materialProps.shininess = 12.0f;

    m_material->SetMaterialProperties(materialProps);

}

pwg::Terrain::~Terrain()
{
}

void pwg::Terrain::Update(float dt)
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

    if (!m_terrainTextures || m_terrainNoiseSettings.dirty || regenerateLayers)
    {
        m_terrainTextures = m_terrainGenerator->GenerateTerrain(m_terrainNoiseSettings, m_size, m_terrainLayersManager->GetLayers());

        m_terrainNoiseSettings.dirty = false;
    }

    /*if (m_terrainNoiseSettings.dirty)
    {
        m_terrainTextures = m_terrainGenerator->GenerateTerrain(m_terrainNoiseSettings, m_size, m_terrainLayersManager->GetLayers());

        m_terrainNoiseSettings.dirty = false;
    }*/

}

void pwg::Terrain::Draw(Renderer& renderer)
{
    
    m_material->Apply();

    auto modelMatrix = m_mesh->GetModelMatrix();

    m_material->SetUniformMat4("u_model", modelMatrix);

    m_material->SetUniformFloat("u_amplitude", m_terrainNoiseSettings.amplitude);


    if (m_terrainTextures && m_terrainTextures->heightmap)
    {
        m_material->SetUniformTexture("u_Heightmap", m_terrainTextures->heightmap, 0);
    }

    if (m_terrainTextures && m_terrainTextures->normalmap)
    {
        m_material->SetUniformTexture("u_Normalmap", m_terrainTextures->normalmap, 1);
    }
    
    if (m_terrainTextures && m_terrainTextures->splatmap)
    {
        m_material->SetUniformTexture("u_Splatmap", m_terrainTextures->splatmap, 2);
    }

    m_mesh->Draw();
}

std::shared_ptr<pwg::Mesh> pwg::Terrain::GetMesh()
{
    return m_mesh;
}

std::shared_ptr<pwg::Material> pwg::Terrain::GetMaterial()
{
    return m_material;
}

int pwg::Terrain::GetSize()
{
    return m_size;
}

pwg::TerrainNoiseSettings& pwg::Terrain::GetNoiseSettings()
{
    return m_terrainNoiseSettings;
}

std::vector<pwg::TerrainLayer>& pwg::Terrain::GetTerrainLayers()
{
    return m_terrainLayersManager->GetLayers();
}

float pwg::Terrain::GetTerrainHeightAt(int x, int z)
{
    auto heightmapSample = m_terrainGenerator->GetCPUHeightmap();
    x = (x - 0.0) / m_size;
    z = (z - 0.0) / m_size;
    return heightmapSample[z * m_size + x] * m_terrainNoiseSettings.amplitude;
}
