#include "renderer.h"

pwg::Renderer::Renderer()
{
	glEnable(GL_DEPTH_TEST);
}

void pwg::Renderer::Clear()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void pwg::Renderer::Draw(IMesh& mesh) 
{
	mesh.Bind();
	glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
	mesh.Unbind();

}
