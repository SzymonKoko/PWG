#ifndef SRC_GRAPHICS_MESH_MESH_H
#define SRC_GRAPHICS_MESH_MESH_H

#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace pwg
{
	/**
	* @brief Struct that represents a single vertex of a mesh.
	* Contains position, normal, texture coordinates, and vertex color.
	*/
	struct Vertex
	{
		glm::vec3 position; /**< Vertex position in 3D space. */
		glm::vec3 normal;   /**< Normal vector for lighting calculations. */
		glm::vec2 uv;       /**< Texture coordinates. */
		glm::vec3 color;    /**< Per-vertex color. */
	};

	/**
	* @brief Class representing a 3D mesh composed of vertices and indices.
	* Handles GPU buffer creation, updates, and rendering.
	*/
	class Mesh
	{
	public:

		/**
		* @brief Constructs a mesh from given vertices and indices.
		* @param vertices Reference to a vector of vertex data.
		* @param indices Reference to a vector of indices.
		* @param size Number of vertices in the mesh.
		*/
		Mesh(const std::vector<pwg::Vertex>& vertices, const std::vector<unsigned int>& indices, int size);

		/**
		* @brief Mesh destructor. Cleans up GPU buffers.
		*/
		~Mesh();

		/**
		* @brief Updates mesh uniforms or data on GPU.
		* @param shaderID ID of the shader program currently in use.
		*/
		void Update(unsigned int shaderID);

		/**
		* @brief Draws the mesh using bound vertex and index buffers.
		*/
		void Draw();

		/**
		* @brief Updates vertex data in the mesh.
		* @param vertices Reference to a vector of new vertex data.
		*/
		void SetVertices(std::vector<Vertex>& vertices);

		/**
		* @brief Updates both vertices and indices of the mesh and resets its size.
		* @param vertices Reference to a vector of vertex data.
		* @param indices Reference to a vector of index data.
		* @param size New mesh size.
		*/
		void UpdateMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int& size);

		/**
		* @brief Getter for vertex data.
		* @return Reference to a vector of mesh vertices.
		*/
		std::vector<Vertex>& GetVertices();

		/**
		* @brief Getter for mesh size.
		* @return Number of vertices in the mesh.
		*/
		int GetSize();

	private:
		std::vector<Vertex> m_vertices;           /**< Vector storing vertex data. */
		std::vector<unsigned int> m_indices;      /**< Vector storing index data. */
		int m_meshSize{ 0 };                      /**< Number of vertices in the mesh. */

		unsigned int m_vaoID{ 0 };                /**< OpenGL Vertex Array Object ID. */
		unsigned int m_vboID{ 0 };                /**< OpenGL Vertex Buffer Object ID. */
		unsigned int m_eboID{ 0 };                /**< OpenGL Element Buffer Object ID. */

		/**
		* @brief Initializes VAO, VBO, and EBO for the mesh and uploads vertex data to GPU.
		*/
		void SetupMesh();
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_MESH_MESH_H
