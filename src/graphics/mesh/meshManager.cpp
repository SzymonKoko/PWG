#include "meshManager.h"
#include <iostream>

namespace pwg
{
	MeshManager::MeshManager()
	{
	}

	MeshManager::~MeshManager()
	{
	}

	std::string MeshManager::CreateMesh(int width, int height, std::string name)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		float topLeftX = (width - 1) / -2.0f;
		float topLeftZ = (height - 1) / 2.0f;

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				Vertex v;
				v.position = glm::vec3((topLeftX + x), 0, (topLeftZ - y));
				v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
				v.uv = glm::vec2(float(x) / (width - 1), float(y) / (height - 1));
				vertices.push_back(v);
			}
		}

		for (int y = 0; y < height-1; y++)
		{
			for (int x = 0; x < width-1; x++)
			{
				unsigned int i0 = y * width + x;	
				unsigned int i1 = i0 + 1;			
				unsigned int i2 = i0 + width;		
				unsigned int i3 = i2 + 1;

				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);

				indices.push_back(i1);
				indices.push_back(i3);
				indices.push_back(i2);
			}
		}

		m_meshes.emplace(name, std::make_unique<Mesh>(vertices, indices));

		return name;
	}

	Mesh& MeshManager::GetMesh(std::string meshID)
	{
		for (auto& mesh : m_meshes)
		{
			if (mesh.first == meshID)
			{
				return *mesh.second;
			}
		}
	}
}

