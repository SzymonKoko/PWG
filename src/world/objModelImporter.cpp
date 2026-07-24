#include "objModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace pwg
{
	namespace
	{
		glm::mat4 ToGlm(const aiMatrix4x4& matrix)
		{
			glm::mat4 result{ 1.0f };
			result[0][0] = matrix.a1;
			result[1][0] = matrix.a2;
			result[2][0] = matrix.a3;
			result[3][0] = matrix.a4;
			result[0][1] = matrix.b1;
			result[1][1] = matrix.b2;
			result[2][1] = matrix.b3;
			result[3][1] = matrix.b4;
			result[0][2] = matrix.c1;
			result[1][2] = matrix.c2;
			result[2][2] = matrix.c3;
			result[3][2] = matrix.c4;
			result[0][3] = matrix.d1;
			result[1][3] = matrix.d2;
			result[2][3] = matrix.d3;
			result[3][3] = matrix.d4;
			return result;
		}

		glm::vec3 ToGlm(const aiColor3D& color)
		{
			return glm::clamp(glm::vec3(color.r, color.g, color.b), glm::vec3(0.0f), glm::vec3(1.0f));
		}

		glm::vec3 TransformNormal(const glm::mat4& transform, const glm::vec3& normal)
		{
			return glm::normalize(glm::mat3(glm::transpose(glm::inverse(transform))) * normal);
		}

		std::string ResolveTexturePath(const std::filesystem::path& modelFolder, const aiString& path)
		{
			std::string raw = path.C_Str();
			if (raw.empty() || raw[0] == '*')
			{
				return {};
			}

			std::filesystem::path texturePath = raw;
			if (!texturePath.is_absolute())
			{
				texturePath = modelFolder / texturePath;
			}
			return texturePath.lexically_normal().generic_string();
		}

		struct ImportedMaterial
		{
			glm::vec3 diffuse{ 0.78f, 0.78f, 0.74f };
			std::string albedoPath;
		};

		ImportedMaterial ReadMaterial(const aiScene* scene, unsigned int materialIndex, const std::filesystem::path& modelFolder)
		{
			ImportedMaterial result;
			if (!scene || materialIndex >= scene->mNumMaterials)
			{
				return result;
			}

			aiMaterial* material = scene->mMaterials[materialIndex];
			aiColor3D diffuse;
			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{
				result.diffuse = ToGlm(diffuse);
			}

			aiColor4D baseColor;
			if (AI_SUCCESS == material->Get(AI_MATKEY_BASE_COLOR, baseColor))
			{
				result.diffuse = glm::clamp(glm::vec3(baseColor.r, baseColor.g, baseColor.b), glm::vec3(0.0f), glm::vec3(1.0f));
			}

			aiString texturePath;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
			{
				result.albedoPath = ResolveTexturePath(modelFolder, texturePath);
			}
			else if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS)
			{
				result.albedoPath = ResolveTexturePath(modelFolder, texturePath);
			}
			return result;
		}

		ModelSubmesh BuildSubmesh(const aiScene* scene, const aiMesh* mesh, const glm::mat4& nodeTransform, const std::filesystem::path& modelFolder, unsigned int meshIndex)
		{
			ImportedMaterial material = ReadMaterial(scene, mesh->mMaterialIndex, modelFolder);
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indices;
			vertices.reserve(mesh->mNumVertices);

			for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
			{
				Vertex vertex{};
				glm::vec4 worldPosition = nodeTransform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
				vertex.position = glm::vec3(worldPosition);
				if (mesh->HasNormals())
				{
					vertex.normal = TransformNormal(nodeTransform, glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
				}
				else
				{
					vertex.normal = { 0.0f, 1.0f, 0.0f };
				}
				if (mesh->HasTextureCoords(0))
				{
					vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
				}
				else
				{
					vertex.uv = { 0.0f, 0.0f };
				}
				vertex.color = material.diffuse;
				vertices.push_back(vertex);
			}

			for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
			{
				const aiFace& face = mesh->mFaces[i];
				if (face.mNumIndices != 3)
				{
					continue;
				}
				indices.push_back(face.mIndices[0]);
				indices.push_back(face.mIndices[1]);
				indices.push_back(face.mIndices[2]);
			}

			ModelSubmesh submesh;
			submesh.name = mesh->mName.length > 0 ? mesh->mName.C_Str() : "Mesh " + std::to_string(meshIndex + 1);
			submesh.diffuseColor = material.diffuse;
			submesh.albedoPath = material.albedoPath;
			if (!vertices.empty() && !indices.empty())
			{
				submesh.mesh = std::make_shared<Mesh>(vertices, indices, static_cast<float>(vertices.size()));
			}
			return submesh;
		}

		void AppendNode(const aiScene* scene, const aiNode* node, const glm::mat4& parentTransform, const std::filesystem::path& modelFolder, ObjImportResult& result)
		{
			glm::mat4 nodeTransform = parentTransform * ToGlm(node->mTransformation);
			for (unsigned int i = 0; i < node->mNumMeshes; ++i)
			{
				unsigned int meshIndex = node->mMeshes[i];
				if (meshIndex >= scene->mNumMeshes)
				{
					continue;
				}

				ModelSubmesh submesh = BuildSubmesh(scene, scene->mMeshes[meshIndex], nodeTransform, modelFolder, meshIndex);
				if (submesh.mesh)
				{
					result.submeshes.push_back(submesh);
				}
			}
			for (unsigned int i = 0; i < node->mNumChildren; ++i)
			{
				AppendNode(scene, node->mChildren[i], nodeTransform, modelFolder, result);
			}
		}
	}

	ObjImportResult ObjModelImporter::Load(const std::string& path)
	{
		ObjImportResult result;
		if (path.empty() || !std::filesystem::exists(path))
		{
			result.message = "Model file does not exist";
			return result;
		}

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path,
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_SortByPType |
			aiProcess_ValidateDataStructure);

		if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
		{
			result.message = std::string("Assimp import failed: ") + importer.GetErrorString();
			return result;
		}

		std::filesystem::path modelFolder = std::filesystem::path(path).parent_path();
		AppendNode(scene, scene->mRootNode, glm::mat4(1.0f), modelFolder, result);

		if (result.submeshes.empty())
		{
			result.message = "Model has no renderable triangle meshes";
			return result;
		}

		result.mesh = result.submeshes.front().mesh;
		result.diffuseColor = result.submeshes.front().diffuseColor;
		result.albedoPath = result.submeshes.front().albedoPath;
		result.success = true;
		result.message = "Loaded model with Assimp: " + std::filesystem::path(path).filename().string() + " (" + std::to_string(result.submeshes.size()) + " mesh parts)";
		return result;
	}
}