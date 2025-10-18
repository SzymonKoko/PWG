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
            if (layer.second.enabled)
            {
                if (v.position.y >= layer.second.minHeight)
                {
                    v.color = layer.second.color;
                }
            }
        }
    }
    m_resourceManager->GetMeshManager().GetMesh("PlaneMesh")->SetVertices(vertices);
}

void pwg::Terrain::Update()
{
	pwg::systems::PlaneMeshSystem::Update(m_registry, m_resourceManager->GetMeshManager());

	if (m_noiseTexture)
	{
		m_noiseTexture->UpdateNoiseData(m_noiseTexture->GetNoiseParameters());
	}
    ApplyNoise();
    ApplyLayers();
}

void pwg::Terrain::AddLayer(const TerrainLayer& terrainLayer)
{
    m_terrainLayers.emplace(terrainLayer.name, terrainLayer);
}

void pwg::Terrain::RemoveLayer(std::string name)
{
    m_terrainLayers.erase(name);
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
        std::cerr << "Brak mesha\n";
    }

    auto mesh = m_resourceManager->GetMeshManager().GetMesh(meshComponent->meshID);

    return mesh;
}

int pwg::Terrain::GetSize()
{
    return m_size;
}
