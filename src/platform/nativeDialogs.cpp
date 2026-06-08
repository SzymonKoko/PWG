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
