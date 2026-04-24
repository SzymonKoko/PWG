#ifndef SRC_TERRAIN_DEBUG_TERRAIN_DEBUG_H_
#define SRC_TERRAIN_DEBUG_TERRAIN_DEBUG_H_

#include <memory>
#include <string>
#include "graphics/texture/texture.h"

namespace pwg
{
	enum class DebugMode
	{
		DEBUG_NONE = 0,
		DEBUG_HEIGHT,
		DEBUG_NORMAL,
		DEBUG_SLOPE
	};

	struct DebugEntry
	{
		std::string name;
		DebugMode mode;
		std::shared_ptr<Texture> texture;
		bool usesTexture;
	};

	struct DebugSettings
	{
		bool wireframe{ false };
	};

	class TerrainDebug
	{
	public:
		TerrainDebug();
		~TerrainDebug();

		void RegisterTexture(std::string name, DebugMode mode, std::shared_ptr<Texture> texture, bool usesTexture);
		void ShowGUI();

		void Clear();

		std::shared_ptr<Texture> GetSelectedTexture();
		std::vector<DebugEntry>& GetEntries();
		DebugSettings& GetDebugSettings();
		int GetDebugMode();
		bool GetStatus();
	private:
		DebugMode m_debugMode = DebugMode::DEBUG_NONE;
		DebugSettings m_debugSettings;
		std::vector<DebugEntry> m_debugEntries;
		int m_selectedTexture{ 0 };
		bool m_debugEnabled{ false };
	};
}
#endif // !SRC_TERRAIN_DEBUG_TERRAIN_DEBUG_H_
