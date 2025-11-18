#include <glad/glad.h>
#include "terrain.h"

pwg::Terrain::Terrain(entt::registry& registry, std::shared_ptr<ResourceManager> resourceManager, int size)
	: m_registry(registry),
	  m_resourceManager(resourceManager),
	  m_size(size)
	  //m_noiseTexture(nullptr)
{
	Entity planeMesh(&m_registry, "PlaneMesh");
	planeMesh.AddComponent<components::MeshComponent>();
	planeMesh.AddComponent<components::PlaneMeshComponent>(size);

	//m_noiseTexture = std::make_shared<NoiseTexture>(size);
    PWG_INFO("Terrain created ({0}x{0})", size);
}

pwg::Terrain::~Terrain()
{
}

void pwg::Terrain::ApplyLayers()
{

}

void pwg::Terrain::SortLayers()
{
    std::sort(m_terrainLayers.begin(), m_terrainLayers.end(),
        [](const TerrainLayer& a, const TerrainLayer& b)
        {
            return a.minHeight < b.minHeight;
        });
}

void pwg::Terrain::Update()
{
	pwg::systems::PlaneMeshSystem::Update(m_registry, m_resourceManager->GetMeshManager());

}

void pwg::Terrain::Draw(Shader& shader, Shader& noise)
{
    
    if (!m_created)
    {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_size, m_size, 0, GL_RGBA, GL_FLOAT, NULL);
        m_created = true;
    }
    
    glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    noise.ActivateShader();
    noise.SetUniformInt("size", m_size);
    glDispatchCompute(m_size/8, m_size/8, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    shader.ActivateShader();
    shader.SetUniformInt("heightmap", 0);

    shader.SetUniformFloat("amplitude", 500);
    m_resourceManager->GetMeshManager().GetMesh("PlaneMesh")->Draw();
}

void pwg::Terrain::AddLayer(const TerrainLayer& terrainLayer)
{
    m_terrainLayers.push_back(terrainLayer);
    PWG_DEBUG("Terrain layer added ({0}, {1}, {2}-{3}, {4}, color: {5} {6} {7})", 
        terrainLayer.name, 
        terrainLayer.enabled, 
        terrainLayer.minHeight, 
        terrainLayer.maxHeight, 
        terrainLayer.textureID,
        terrainLayer.color.x,
        terrainLayer.color.y,
        terrainLayer.color.z
    );
    SortLayers();
}

void pwg::Terrain::RemoveLayer(std::string name)
{
    //m_terrainLayers.erase(name);
    PWG_DEBUG("Removed terrain layer {0}", name);
    SortLayers();
}

std::shared_ptr<pwg::Mesh> pwg::Terrain::GetMesh()
{
    pwg::components::MeshComponent* meshComponent = nullptr;
    pwg::components::PlaneMeshComponent* planeMeshComponent = nullptr;
    auto meshView = m_registry.view<components::MeshComponent, components::PlaneMeshComponent>();
    for (auto [entity, mesh, plane] : meshView.each())
    {
        meshComponent = &mesh;
        planeMeshComponent = &plane;
        break;
    }

    if (!meshComponent)
    {
        PWG_ERROR("Brak mesha");
    }

    auto mesh = m_resourceManager->GetMeshManager().GetMesh(meshComponent->meshID);

    return mesh;
}

std::vector<pwg::TerrainLayer>& pwg::Terrain::GetTerrainLayers()
{
    return m_terrainLayers;
}

int pwg::Terrain::GetSize()
{
    return m_size;
}
