#include "sunObject.h"

namespace pwg
{
	SunObject::SunObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
		: m_mesh(mesh),
		  m_material(material)
	{
	}

	SunObject::~SunObject()
	{
	}

	void SunObject::Update(float dt)
	{
		m_time += dt;
		float offsetY = 1400.0f;

		m_position.x = cos(m_time * m_orbitSpeed) * m_orbitRadius;
		m_position.y = sin(m_time * m_orbitSpeed) * m_orbitRadius + offsetY;
		m_position.z = 0.0f;

		m_mesh->SetPosition(m_position);
		m_mesh->Update();
	}

	void SunObject::Draw(Renderer& renderer)
	{
		m_material->Apply();

		auto modelMatrix = m_mesh->GetModelMatrix();
		
		m_material->SetUniformMat4("u_model", modelMatrix);
		m_material->SetUniformVec3("u_Color", m_color);

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
		return m_position;
	}
	glm::vec3 SunObject::GetColor()
	{
		return m_color;
	}
}

