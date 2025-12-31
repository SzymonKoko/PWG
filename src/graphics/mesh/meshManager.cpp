#define _USE_MATH_DEFINES

#include "meshManager.h"
#include "core/logger/logger.h"
#include <iostream>
#include <cmath>

namespace pwg
{
	MeshManager::MeshManager()
	{
	}

	MeshManager::~MeshManager()
	{
	}

	std::string MeshManager::CreatePlaneMesh(int size, std::string name)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		// 1000000
		// 5988006
		vertices.resize(size * size);
		indices.resize(((size - 1) * (size - 1) * 6));

		uint16_t width = size;
		uint16_t height = size;

		float topLeftX = (width - 1) / -2.0f;
		float topLeftZ = (height - 1) / 2.0f;

		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				uint32_t idx = y * width + x;
				vertices[idx].position = glm::vec3((topLeftX + x), 0, (topLeftZ - y));
				vertices[idx].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				vertices[idx].uv = glm::vec2(float(x) / (width - 1), float(y) / (height - 1));
			}
		}

		for (uint32_t y = 0; y < height-1; y++)
		{
			for (uint32_t x = 0; x < width-1; x++)
			{
				uint32_t idx = (y * (width - 1) + x) * 6;
				uint32_t i0 = y * width + x;
				uint32_t i1 = i0 + 1;
				uint32_t i2 = i0 + width;
				uint32_t i3 = i2 + 1;

				indices[idx] = i0;
				indices[idx+1] = i1;
				indices[idx+2] = i2;

				indices[idx+3] = i1;
				indices[idx+4] = i3;
				indices[idx+5] = i2;
			}
		}

		m_meshes.emplace(name, std::make_shared<Mesh>(vertices, indices, (float)size));

		PWG_INFO("Created mesh \"{0}\" ({1}x{1})", name, size);
		return name;
	}

	std::string MeshManager::CreateSphereMesh(float radius, int sectorCount, int sliceCount, std::string name)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		vertices.resize((sectorCount+1) * (sliceCount+1));
		indices.resize(((sectorCount) * (sliceCount) * 6));

		float sectorStep = (2 * M_PI) / sectorCount;
		float sliceStep = M_PI / sliceCount;

		float sectorAngle = 0;
		float sliceAngle = 0;

		for (int i = 0; i <= sliceCount; i++)
		{
			sliceAngle = M_PI / 2 - i * sliceStep;
			float xy = radius * cosf(sliceAngle);
			float z = radius * sinf(sliceAngle);

			for (int j = 0; j <= sectorCount; j++)
			{
				sectorAngle = j * sectorStep;

				float x = xy * cosf(sectorAngle);
				float y = xy * sinf(sectorAngle);

				uint32_t idx = i * (sectorCount + 1) + j;
				vertices[idx].position = glm::vec3(x, y, z);
				vertices[idx].normal = glm::normalize(glm::vec3(x, y, z));
				vertices[idx].uv = glm::vec2((float)j / sectorCount, (float)i / sliceCount);
			}
		}

		uint32_t idx = 0;
		for (int i = 0; i < sliceCount; i++)
		{

			for (int j = 0; j < sectorCount; j++)
			{
				int k1 = i * (sectorCount+1) + j;
				int k2 = k1 + sectorCount+1;

				indices[idx++] = k1;
				indices[idx++] = k2;
				indices[idx++] = k1 + 1;
						 
				indices[idx++] = k1 + 1;
				indices[idx++] = k2;
				indices[idx++] = k2 + 1;

			}
		}

		m_meshes.emplace(name, std::make_shared<Mesh>(vertices, indices, radius));

		PWG_INFO("Created sphere mesh \"{0}\" (Radius: {1}, Sectors: {1}, Slices: {2})", name, radius, sectorCount, sliceCount);
		return name;
	}

	std::shared_ptr<Mesh> MeshManager::GetMesh(std::string meshID)
	{
		if (!m_meshes.contains(meshID))
		{
			return nullptr;
		}
		return m_meshes[meshID];
	}
}

