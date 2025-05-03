#ifndef INDEX_BUFFER_H_
#define INDEX_BUFFER_H_

#include <glad.h>
#include <vector>

namespace pwg
{
	class IndexBuffer
	{
	public:

		// Constructor that generates a Index Buffer Object and links it to indices
		IndexBuffer(std::vector<GLuint>& indices);
		~IndexBuffer() = default;

		// Binds the EBO
		void Bind();
		// Unbinds the EBO
		void Unbind();
		// Deletes the EBO
		void Delete();

	private:
		GLuint m_indexBufferObjectID{ 0 };
	};
} // namespace pwg
#endif // !INDEX_BUFFER_H_
