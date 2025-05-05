#include "renderer.h"

void pwg::Renderer::Clear()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void pwg::Renderer::Draw(IMesh& mesh) 
{
	//std::cout << "IndexCount: " << mesh.GetIndexCount();
	mesh.Bind();
	glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
	mesh.Unbind();

}
