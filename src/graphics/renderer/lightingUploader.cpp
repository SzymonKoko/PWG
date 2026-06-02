#include "lightingUploader.h"

void pwg::LightingUploader::Upload(std::shared_ptr<Material> material, Light& light, glm::vec3 cameraPosition)
{
	material->SetUniformVec3("u_lightPosition", light.position);
	material->SetUniformVec3("u_lightDirection", light.direction);
	material->SetUniformVec3("u_lightColor", light.color);
	material->SetUniformFloat("u_lightIntensity", light.intensivity);
	material->SetUniformVec3("u_cameraPosition", cameraPosition);
	material->SetUniformFloat("u_exposure", light.exposure);
	material->SetUniformFloat("u_skyAmbient", light.skyAmbient);
	material->SetUniformFloat("u_groundAmbient", light.groundAmbient);
	material->SetUniformFloat("u_normalStrength", light.normalStrength);
	material->SetUniformFloat("u_snowHeight", light.snowHeight);
	material->SetUniformFloat("u_snowSoftness", light.snowSoftness);
	material->SetUniformFloat("u_wetnessStrength", light.wetnessStrength);
	material->SetUniformFloat("u_waterAlpha", light.waterAlpha);
}
