#include "lightingUploader.h"

void pwg::LightingUploader::Upload(std::shared_ptr<Material> material, Light& light)
{

	material->SetUniformVec3("u_light.direction", light.direction);
	material->SetUniformVec3("u_light.color", light.color);
	material->SetUniformFloat("u_light.intensivity", light.intensivity);

}
