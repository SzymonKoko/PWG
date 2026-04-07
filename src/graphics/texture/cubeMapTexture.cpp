#include <glad/glad.h>
#include "cubeMapTexture.h"

namespace pwg
{
	CubeMapTexture::CubeMapTexture(std::vector<std::string> facesPaths)
	{
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		glGenTextures(1, &m_cubeMapID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapID);

		stbi_set_flip_vertically_on_load(false);

		for (unsigned int i = 0; i < facesPaths.size(); i++)
		{
			m_image = stbi_load(facesPaths[i].c_str(), &m_width, &m_height, &m_nrChannels, 0);
			if (m_image)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_image
				);
				stbi_image_free(m_image);
			}
			else
			{
				PWG_ERROR("Cubemap tex failed to load at path: {}", facesPaths[i]);
				stbi_image_free(m_image);
			}
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		PWG_INFO("CUBEMAP Created!");

		stbi_set_flip_vertically_on_load(true);
	}

	void CubeMapTexture::Bind(int slot)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapID);
	}

	void CubeMapTexture::Unbind()
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	unsigned int CubeMapTexture::GetCubeMapID()
	{
		return m_cubeMapID;
	}

}
