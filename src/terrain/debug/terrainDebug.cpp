#include "terrainDebug.h"
#include "imgui.h"

namespace pwg
{
	TerrainDebug::TerrainDebug()
	{

	}
	TerrainDebug::~TerrainDebug()
	{

	}
	void TerrainDebug::RegisterTexture(std::string name, DebugMode mode, std::shared_ptr<Texture> texture, bool usesTexture)
	{

		m_debugEntries.push_back({ name, mode, texture, usesTexture });
		
		PWG_INFO("Added debug texture {} to the map", name);
	}
	void TerrainDebug::ShowGUI()
	{
		std::vector<const char*> textureNames;
		for (const auto& entry : m_debugEntries)
		{
			textureNames.push_back(entry.name.c_str());
		}

		ImGui::Checkbox("Wireframe", &m_debugSettings.wireframe);
		ImGui::Separator();
		ImGui::Checkbox("Enable Debug", &m_debugEnabled);

		ImGui::Combo("Debug Modes", &m_selectedTexture, textureNames.data(), textureNames.size());
	}
	void TerrainDebug::Clear()
	{
		m_debugEntries.clear();
	}
	std::shared_ptr<Texture> TerrainDebug::GetSelectedTexture()
	{
		return m_debugEntries[m_selectedTexture].texture;
	}
	std::vector<DebugEntry>& TerrainDebug::GetEntries()
	{
		return m_debugEntries;
	}
	DebugSettings& TerrainDebug::GetDebugSettings()
	{
		return m_debugSettings;
	}
	int TerrainDebug::GetDebugMode()
	{
		return m_selectedTexture;
	}
	bool TerrainDebug::GetStatus()
	{
		return m_debugEnabled;
	}
}

