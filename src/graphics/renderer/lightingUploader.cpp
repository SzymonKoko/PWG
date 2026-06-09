#include "lightingUploader.h"

void pwg::LightingUploader::Upload(std::shared_ptr<Material> material, const EnvironmentLighting& lighting, glm::vec3 cameraPosition)
{
	material->SetUniformVec3("u_lightPosition", -lighting.sunDirection * 100000.0f);
	material->SetUniformVec3("u_lightDirection", lighting.sunDirection);
	material->SetUniformVec3("u_lightColor", lighting.sunColor);
	material->SetUniformFloat("u_lightIntensity", lighting.sunIntensity);
	material->SetUniformVec3("u_cameraPosition", cameraPosition);
	material->SetUniformFloat("u_exposure", 1.0f);
	material->SetUniformFloat("u_skyAmbient", lighting.skyAmbient);
	material->SetUniformFloat("u_groundAmbient", lighting.groundAmbient);
	material->SetUniformVec3("u_skyTint", lighting.skyTint);
	material->SetUniformVec3("u_groundAlbedo", lighting.groundAlbedo);
	material->SetUniformFloat("u_fogDensity", lighting.fogDensity);
	material->SetUniformFloat("u_normalStrength", lighting.normalStrength);
	material->SetUniformFloat("u_snowHeight", lighting.snowHeight);
	material->SetUniformFloat("u_snowSoftness", lighting.snowSoftness);
	material->SetUniformFloat("u_wetnessStrength", lighting.wetnessStrength);
	material->SetUniformFloat("u_waterAlpha", lighting.waterAlpha);
}
