#ifndef SRC_GRAPHICS_MESH_MESH_H
#define SRC_GRAPHICS_MESH_MESH_H

#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <graphics/common/transform.h>

namespace pwg
{
	/**
	* @brief Struct that represents a single vertex of a mesh.
	* Contains position, normal, texture coordinates, and vertex color.
	*/
	struct Vertex
	{
		glm::vec3 position; /**< Vertex position in 3D space */
		glm::vec3 normal;   /**< Normal vector used for lighting calculations */
		glm::vec2 uv;       /**< Texture coordinates */
		glm::vec3 color;    /**< Per-vertex color */
	};

	/**
	* @brief Class representing a 3D mesh composed of vertices and indices.
	* Responsible for GPU buffer management and rendering.
	*/
	class Mesh
	{
	public:

		/**
		* @brief Constructs a mesh from given vertices and indices.
		* @param vertices Vector containing vertex data.
		* @param indices Vector containing index data.
		* @param size Number of vertices in the mesh.
		*/
		Mesh(const std::vector<pwg::Vertex>& vertices, const std::vector<unsigned int>& indices, float size);

		/**
		* @brief Destructor for the mesh.
		* Releases allocated GPU resources.
		*/
		~Mesh();

		/**
		* @brief Updates mesh-related data on the GPU.
		* Typically called once per frame.
		*/
		void Update();

		/**
		* @brief Renders the mesh using currently bound shader and buffers.
		*/
		void Draw();

		/**
		* @brief Replaces the mesh vertex data.
		* @param vertices Vector containing new vertex data.
		*/
		void SetVertices(std::vector<Vertex>& vertices);

		/**
		* @brief Updates vertex and index data and resets mesh size.
		* @param vertices Vector containing new vertex data.
		* @param indices Vector containing new index data.
		* @param size New mesh size.
		*/
		void UpdateMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int& size);

		/**
		* @brief Returns a reference to the vertex data.
		* @return Vector of vertices.
		*/
		std::vector<Vertex>& GetVertices();

		/**
		* @brief Computes and returns the model matrix for this mesh.
		* @return Model transformation matrix.
		*/
		glm::mat4 GetModelMatrix();

		/**
		* @brief Returns the world-space position of the mesh.
		* @return Mesh position.
		*/
		glm::vec3 GetPosition();

		int GetVertexCount();

		/**
		* @brief Sets the world-space position of the mesh.
		* @param position New mesh position.
		*/
		void SetPosition(glm::vec3 position);

		/**
		* @brief Returns the mesh size.
		* @return Number of vertices in the mesh.
		*/
		int GetSize();

	private:
		std::vector<Vertex> m_vertices;      /**< Stored vertex data */
		std::vector<unsigned int> m_indices; /**< Stored index data */
		int m_meshSize{ 0 };                 /**< Number of vertices */

		unsigned int m_vaoID{ 0 };           /**< OpenGL Vertex Array Object ID */
		unsigned int m_vboID{ 0 };           /**< OpenGL Vertex Buffer Object ID */
		unsigned int m_eboID{ 0 };           /**< OpenGL Element Buffer Object ID */

		int m_modelUniformLocation{ -1 };    /**< Cached model matrix uniform location */

		glm::mat4 m_model{ 1.0f };           /**< Model transformation matrix */
		Transform m_transform;               /**< Local transform data */

		/**
		* @brief Initializes GPU buffers and uploads mesh data.
		* Creates VAO, VBO, and EBO objects.
		*/
		void SetupMesh();
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_MESH_MESH_H
