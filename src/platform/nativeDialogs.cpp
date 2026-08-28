#include "nativeDialogs.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <filesystem>
namespace
{
	std::string AssetInitialDirectory()
	{
		std::filesystem::path runtimeAssets = std::filesystem::absolute("../assets");
		if (std::filesystem::is_directory(runtimeAssets))
		{
			return runtimeAssets.string();
		}

		std::filesystem::path sourceAssets = std::filesystem::absolute("assets");
		if (std::filesystem::is_directory(sourceAssets))
		{
			return sourceAssets.string();
		}

		return {};
	}

	int CALLBACK BrowseFolderCallback(HWND hwnd, UINT message, LPARAM, LPARAM data)
	{
		if (message == BFFM_INITIALIZED && data != 0)
		{
			SendMessageA(hwnd, BFFM_SETSELECTIONA, TRUE, data);
		}
		return 0;
	}
}
namespace pwg::platform
{
	std::string OpenProjectDialog()
	{
		char fileName[MAX_PATH] = {};
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "PWG Project (*.pwg)\0*.pwg\0All Files (*.*)\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return fileName;
		}
		return {};
	}

	std::string SaveProjectDialog()
	{
		char fileName[MAX_PATH] = {};
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "PWG Project (*.pwg)\0*.pwg\0All Files (*.*)\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrDefExt = "pwg";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return fileName;
		}
		return {};
	}

	std::string OpenImageDialog()
	{
		char fileName[MAX_PATH] = {};
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "Image Files (*.jpg;*.jpeg;*.png;*.tga;*.hdr)\0*.jpg;*.jpeg;*.png;*.tga;*.hdr\0All Files (*.*)\0*.*\0";
		ofn.nFilterIndex = 1;
		std::string initialDir = AssetInitialDirectory();
		if (!initialDir.empty())
		{
			ofn.lpstrInitialDir = initialDir.c_str();
		}
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return fileName;
		}
		return {};
	}
	std::string OpenObjDialog()
	{
		char fileName[MAX_PATH] = {};
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "Model Files (*.obj;*.fbx;*.dae;*.gltf;*.glb;*.3ds;*.blend)\0*.obj;*.fbx;*.dae;*.gltf;*.glb;*.3ds;*.blend\0All Files (*.*)\0*.*\0";
		ofn.nFilterIndex = 1;
		std::string initialDir = AssetInitialDirectory();
		if (!initialDir.empty())
		{
			ofn.lpstrInitialDir = initialDir.c_str();
		}
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return fileName;
		}
		return {};
	}
	std::string OpenVegetationModelDialog()
	{
		char fileName[MAX_PATH] = {};
		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "glTF Models (*.gltf;*.glb)\0*.gltf;*.glb\0";
		ofn.nFilterIndex = 1;
		std::string initialDir = AssetInitialDirectory();
		if (!initialDir.empty())
		{
			ofn.lpstrInitialDir = initialDir.c_str();
		}
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return fileName;
		}
		return {};
	}
	std::string PickFolderDialog(const char* title)
	{
		BROWSEINFOA browseInfo = {};
		browseInfo.lpszTitle = title;
		browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		std::string initialDir = AssetInitialDirectory();
		if (!initialDir.empty())
		{
			browseInfo.lpfn = BrowseFolderCallback;
			browseInfo.lParam = reinterpret_cast<LPARAM>(initialDir.c_str());
		}
		LPITEMIDLIST item = SHBrowseForFolderA(&browseInfo);
		if (!item)
		{
			return {};
		}

		char path[MAX_PATH] = {};
		SHGetPathFromIDListA(item, path);
		CoTaskMemFree(item);
		return path;
	}
}

