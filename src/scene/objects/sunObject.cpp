#include "sunObject.h"

namespace pwg
{
	SunObject::SunObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
		: m_mesh(mesh),
		  m_material(material)
	{
		m_light.color = glm::vec3(1.0f, 1.0f, 1.0f);
		m_light.intensivity = 1.0f;
		m_sunObjectProperties.orbitSpeed = 0.05f;
		m_sunObjectProperties.orbitRadius = 5000.0;
		
	}

	SunObject::~SunObject()
	{
	}

	void SunObject::Update(float dt, std::shared_ptr<ICamera> camera)
	{
		if (!m_sunObjectProperties.stopTime)
		{
			m_sunObjectProperties.time += dt;
		}

		m_sunObjectProperties.position.x = cos(m_sunObjectProperties.time * m_sunObjectProperties.orbitSpeed) * m_sunObjectProperties.orbitRadius;
		m_sunObjectProperties.position.y = sin(m_sunObjectProperties.time * m_sunObjectProperties.orbitSpeed) * m_sunObjectProperties.orbitRadius;
		m_sunObjectProperties.position.z = sin(m_sunObjectProperties.time * m_sunObjectProperties.orbitSpeed) * m_sunObjectProperties.orbitRadius;

		m_light.position = m_sunObjectProperties.position;

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

	void SunObject::SetLight(Light& light)
	{
		m_light = light;
	}

	SunObjectProperties& SunObject::GetSunObjectProperties()
	{
		return m_sunObjectProperties;
	}
	void SunObject::SetSunObjectProperties(SunObjectProperties& properties)
	{
		m_sunObjectProperties = properties;
	}
}

