#include "terrain.h"

pwg::Terrain::Terrain(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, std::shared_ptr<ComputeShader> noiseComputeShader)
    : m_mesh(mesh),
      m_material(material)
{
    m_size = 512;

    m_terrainGenerator = std::make_unique<TerrainGenerator>(noiseComputeShader);
}

pwg::Terrain::~Terrain()
{
}

void pwg::Terrain::Update()
{
    if (m_terrainNoiseSettings.dirty)
    {
        m_terrainTextures = m_terrainGenerator->GenerateTerrain(m_terrainNoiseSettings, m_size);

        m_terrainNoiseSettings.dirty = false;
    }
}

void pwg::Terrain::Draw(Renderer& renderer)
{
    
    m_material->Apply();

    glm::mat4 modelMatrix = m_mesh->GetModelMatrix();

    m_material->SetUniformMat4("u_model", modelMatrix);

    m_material->SetUniformTexture("u_heightmap", m_terrainTextures->heightmap, 0);
    m_material->SetUniformTexture("u_normalmap", m_terrainTextures->normalmap, 1);
    m_material->SetUniformTexture("u_splatmap", m_terrainTextures->splatmap, 2);

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

int pwg::Terrain::GetSize()
{
    return m_size;
}

pwg::TerrainNoiseSettings& pwg::Terrain::GetNoiseSettings()
{
    return m_terrainNoiseSettings;
}
