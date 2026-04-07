#include "skybox.h"

namespace pwg
{
	SkyBox::SkyBox(std::shared_ptr<CubeMapTexture> cubemap, std::shared_ptr<Material> material)
		: m_material(material),
		  m_cubeMapTexture(cubemap)
	{
		std::vector<Vertex> vertices;

		for (int i = 0; i < 108; i += 3)
		{
			Vertex v;
			v.position = glm::vec3(m_skyboxVertices[i], m_skyboxVertices[i + 1], m_skyboxVertices[i + 2]);
			v.normal = glm::vec3(0.0f);
			v.uv = glm::vec2(0.0f);
			v.color = glm::vec3(1.0f);

			vertices.push_back(v);
		}

		m_mesh = std::make_shared<Mesh>(vertices);
	}

	void SkyBox::Draw()
	{

		m_material->SetCubeMapTexture("u_skybox", m_cubeMapTexture);

		m_material->Apply();

		m_mesh->Draw();
	}

	std::shared_ptr<Material> SkyBox::GetMaterial()
	{
		return m_material;
	}


}

