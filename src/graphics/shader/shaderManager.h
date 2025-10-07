#ifndef SRC_GRAPHICS_SHADER_SHADER_MANAGER_H
#define SRC_GRAPHICS_SHADER_SHADER_MANAGER_H

#include "shader.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace pwg
{
	class ShaderManager
	{
	public:
		ShaderManager() = default;
		~ShaderManager() = default;

		void Load(const std::string& shaderID, const std::string& vertexPath, const std::string& fragmentPath);
		void Unload(std::string& shaderID);
		void UnloadAll();
		std::shared_ptr<Shader> GetShader(const std::string& shaderID);
	private:
		std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
	};
}
#endif // !SRC_GRAPHICS_SHADER_SHADER_MANAGER_H
