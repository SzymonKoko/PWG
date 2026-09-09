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
	material->SetUniformInt("u_useSkybox", lighting.useSkyboxBlend ? 1 : 0);
	material->SetUniformFloat("u_skyboxBlend", lighting.skyboxBlend);
	material->SetUniformVec3("u_groundAlbedo", lighting.groundAlbedo);
	material->SetUniformFloat("u_fogDensity", lighting.fogDensity);
	material->SetUniformFloat("u_normalStrength", lighting.normalStrength);
	material->SetUniformFloat("u_snowHeight", lighting.snowHeight);
	material->SetUniformFloat("u_snowSoftness", lighting.snowSoftness);
	material->SetUniformFloat("u_wetnessStrength", lighting.wetnessStrength);
	material->SetUniformFloat("u_waterAlpha", lighting.waterAlpha);
	material->SetUniformVec3("u_waterShallowColor", lighting.waterShallowColor);
	material->SetUniformVec3("u_waterDeepColor", lighting.waterDeepColor);
	material->SetUniformFloat("u_waterNormalStrength", lighting.waterNormalStrength);
	material->SetUniformFloat("u_waterWaveScale", lighting.waterWaveScale);
	material->SetUniformFloat("u_waterWaveSpeed", lighting.waterWaveSpeed);
	material->SetUniformFloat("u_waterRefractionStrength", lighting.waterRefractionStrength);
	material->SetUniformFloat("u_waterAbsorption", lighting.waterAbsorption);
	material->SetUniformFloat("u_waterFoamWidth", lighting.waterFoamWidth);
	material->SetUniformFloat("u_waterFoamIntensity", lighting.waterFoamIntensity);
	material->SetUniformFloat("u_waterRoughness", lighting.waterRoughness);
}
