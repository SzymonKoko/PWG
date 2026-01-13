#include "sunObject.h"

namespace pwg
{
	SunObject::SunObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
		: m_mesh(mesh),
		  m_material(material)
	{
		m_light.color = glm::vec3{ 1.0f };
		m_light.intensivity = 0.7f;
		
	}

	SunObject::~SunObject()
	{
	}

	void SunObject::Update(float dt)
	{
		m_sunObjectProperties.time += dt;
		float offsetY = 1000.0f;

		m_sunObjectProperties.position.x = cos(m_sunObjectProperties.time * m_sunObjectProperties.orbitSpeed) * m_sunObjectProperties.orbitRadius;
		m_sunObjectProperties.position.y = sin(m_sunObjectProperties.time * m_sunObjectProperties.orbitSpeed) * m_sunObjectProperties.orbitRadius + offsetY;
		m_sunObjectProperties.position.z = 0.0f;// sin(m_time * m_orbitSpeed * 0.5f)* m_orbitRadius;

		m_light.direction = -glm::normalize(m_sunObjectProperties.position);
		m_mesh->SetPosition(m_sunObjectProperties.position);
		m_mesh->Update();
	}

	void SunObject::Draw(Renderer& renderer)
	{
		m_material->Apply();

		auto modelMatrix = m_mesh->GetModelMatrix();
		
		m_material->SetUniformMat4("u_model", modelMatrix);
		m_material->SetUniformVec3("u_Color", m_sunObjectProperties.color);

		m_mesh->Draw();
	}

	std::shared_ptr<Mesh> SunObject::GetMesh()
	{
		return m_mesh;
	}

	std::shared_ptr<Material> SunObject::GetMaterial()
	{
		return m_material;
	}

	glm::vec3 SunObject::GetPosition()
	{
		return m_sunObjectProperties.position;
	}

	glm::vec3 SunObject::GetColor()
	{
		return m_sunObjectProperties.color;
	}

	Light& SunObject::GetLight()
	{
		return m_light;
	}

	SunObjectProperties& SunObject::GetSunObjectProperties()
	{
		return m_sunObjectProperties;
	}
}

