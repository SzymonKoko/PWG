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
}

pwg::Terrain::~Terrain()
{
}

void pwg::Terrain::Update(float dt)
{
    m_mesh->Update();

    if (!m_terrainTextures)
    {
        m_terrainTextures = m_terrainGenerator->GenerateTerrain(m_terrainNoiseSettings, m_size, m_terrainLayersManager->GetLayers());

        m_terrainNoiseSettings.dirty = false;
    }

    if (m_terrainNoiseSettings.dirty)
    {
        m_terrainTextures = m_terrainGenerator->GenerateTerrain(m_terrainNoiseSettings, m_size, m_terrainLayersManager->GetLayers());

        m_terrainNoiseSettings.dirty = false;
    }

}

void pwg::Terrain::Draw(Renderer& renderer)
{
    
    m_material->Apply();

    auto modelMatrix = m_mesh->GetModelMatrix();

    m_material->SetUniformMat4("u_model", modelMatrix);

    m_material->SetUniformFLoat("u_amplitude", m_terrainNoiseSettings.amplitude);


    if (m_terrainTextures && m_terrainTextures->heightmap)
    {
        m_material->SetUniformTexture("u_Heightmap", m_terrainTextures->heightmap, 0);
    }
    //m_material->SetUniformTexture("u_normalmap", m_terrainTextures->normalmap, 1);
    if (m_terrainTextures && m_terrainTextures->splatmap)
    {
        m_material->SetUniformTexture("u_Splatmap", m_terrainTextures->splatmap, 1);
    }

    

    m_mesh->Draw();


    /*if (!m_created)
    {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size, m_size, 0, GL_RGBA, GL_FLOAT, NULL);
        m_created = true;
    }
    
    noise.ActivateShader();
    noise.SetUniformInt("size", m_size);
    float amplitude = noise.GetUniformFloat("amplitude");
    noise.BindImage(0, m_texture, GL_READ_WRITE, GL_RGBA32F);
    noise.DispatchForTexture(m_size, m_size);
    noise.MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);


    noise.BindTextureSampler(0, m_texture);

    shader.ActivateShader();
    shader.SetUniformInt("heightmap", 0);


    shader.SetUniformFloat("amplitude", amplitude);
    m_resourceManager->GetMeshManager().GetMesh("PlaneMesh")->Draw();*/
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
