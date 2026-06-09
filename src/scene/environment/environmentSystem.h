#ifndef SRC_SCENE_ENVIRONMENT_ENVIRONMENT_SYSTEM_H_
#define SRC_SCENE_ENVIRONMENT_ENVIRONMENT_SYSTEM_H_

#include "environmentSettings.h"

namespace pwg
{
	class EnvironmentSystem
	{
	public:
		EnvironmentSystem();

		void Update(float dt);
		void ResetToDefaults();

		EnvironmentSettings& GetSettings();
		const EnvironmentSettings& GetSettings() const;
		EnvironmentLighting GetLighting() const;

	private:
		void RecalculateSun();

		EnvironmentSettings m_settings;
	};
}

#endif // SRC_SCENE_ENVIRONMENT_ENVIRONMENT_SYSTEM_H_
