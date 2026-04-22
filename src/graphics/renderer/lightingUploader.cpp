#include "lightingUploader.h"

void pwg::LightingUploader::Upload(std::shared_ptr<Material> material, Light& light, glm::vec3 cameraPosition)
{
	material->SetUniformVec3("u_lightPosition", light.position);
	material->SetUniformVec3("u_lightDirection", light.direction);
	material->SetUniformVec3("u_lightColor", light.color);
	material->SetUniformFloat("u_lightIntensity", light.intensivity);
	material->SetUniformVec3("u_cameraPosition", cameraPosition);
}
