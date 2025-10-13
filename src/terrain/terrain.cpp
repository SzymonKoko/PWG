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

	m_noiseTexture = std::make_shared<NoiseTexture>(size); //Add size to constructor
}

pwg::Terrain::~Terrain()
{
}

void pwg::Terrain::ApplyNoise()
{
    NoiseDeformer::ApplyNoise(*m_resourceManager->GetMeshManager().GetMesh("PlaneMesh"), m_noiseTexture->GetNoiseData());
}

void pwg::Terrain::Update()
{
	pwg::systems::PlaneMeshSystem::Update(m_registry, m_resourceManager->GetMeshManager());

	if (m_noiseTexture)
	{
		m_noiseTexture->UpdateNoiseData(m_noiseTexture->GetNoiseParameters());
	}
    ApplyNoise();
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
