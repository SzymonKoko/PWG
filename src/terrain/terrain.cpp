#include "terrain.h"

pwg::Terrain::Terrain(entt::registry& registry, std::shared_ptr<ResourceManager> resourceManager, int size)
	: m_registry(registry),
	  m_resourceManager(resourceManager),
	  m_size(size),
	  m_noiseTexture(nullptr)
{
	Entity planeMesh(&m_registry, "PlaneMesh");
	planeMesh.AddComponent<components::MeshComponent>();
	planeMesh.AddComponent<components::PlaneMeshComponent>(size);

	m_noiseTexture = std::make_shared<NoiseTexture>(size);
    PWG_INFO("Terrain created ({0}x{0})", size);
}

pwg::Terrain::~Terrain()
{
}

void pwg::Terrain::ApplyNoise()
{
    NoiseDeformer::ApplyNoise(*m_resourceManager->GetMeshManager().GetMesh("PlaneMesh"), m_noiseTexture->GetNoiseData());
}

void pwg::Terrain::ApplyLayers()
{
    std::vector<Vertex> vertices = m_resourceManager->GetMeshManager().GetMesh("PlaneMesh")->GetVertices();
    for (auto& v : vertices)
    {
        for (auto layer : m_terrainLayers)
        {
            if (layer.enabled)
            {
                /*if (v.position.y >= layer.minHeight && v.position.y <= layer.maxHeight)
                {*/
                    v.color = BlendColors(v.position.y);
                //}
            }
        }
    }
    m_resourceManager->GetMeshManager().GetMesh("PlaneMesh")->SetVertices(vertices);
}

void pwg::Terrain::SortLayers()
{
    std::sort(m_terrainLayers.begin(), m_terrainLayers.end(),
        [](const TerrainLayer& a, const TerrainLayer& b)
        {
            return a.minHeight < b.minHeight;
        });
}

void pwg::Terrain::FixBoundaries()
{
    for (int i = 1; i < m_terrainLayers.size(); i++)
    {
        if (m_terrainLayers[i].enabled && m_terrainLayers[i - 1].enabled)
        {
            m_terrainLayers[i].minHeight = m_terrainLayers[i - 1].maxHeight;
            if (m_terrainLayers[i].minHeight >= m_terrainLayers[i].maxHeight)
            {
                m_terrainLayers[i].maxHeight = m_terrainLayers[i].minHeight + 0.01f;
            }
        }
        
    }
}

glm::vec3 pwg::Terrain::BlendColors(float height)
{
    glm::vec3 color = {0.0f, 0.0f, 0.0f};

    for (int i = 0; i < m_terrainLayers.size(); i++)
    {
        const auto& lowerLayer = m_terrainLayers[i];
        const auto& upperLayer = m_terrainLayers[i + 1];

        if (height >= lowerLayer.minHeight && height <= lowerLayer.maxHeight)
        {
            float normalized = (height - lowerLayer.maxHeight) / (upperLayer.minHeight - lowerLayer.maxHeight);
            normalized = std::clamp(normalized, 0.0f, 1.0f);
            color = glm::mix(lowerLayer.color, upperLayer.color, normalized);
        }
    }


    return color;
}

void pwg::Terrain::NormalizeHeight()
{

}

void pwg::Terrain::Update()
{
	pwg::systems::PlaneMeshSystem::Update(m_registry, m_resourceManager->GetMeshManager());

	if (m_noiseTexture)
	{
		m_noiseTexture->UpdateNoiseData(m_noiseTexture->GetNoiseParameters());
	}
    ApplyNoise();
    SortLayers();
    FixBoundaries();
    ApplyLayers();
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
    FixBoundaries();
}

void pwg::Terrain::RemoveLayer(std::string name)
{
    //m_terrainLayers.erase(name);
    PWG_DEBUG("Removed terrain layer {0}", name);
    SortLayers();
    FixBoundaries();
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
