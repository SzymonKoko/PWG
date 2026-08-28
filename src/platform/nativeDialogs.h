#ifndef SRC_PLATFORM_NATIVE_DIALOGS_H_
#define SRC_PLATFORM_NATIVE_DIALOGS_H_

#include <string>

namespace pwg::platform
{
	std::string OpenProjectDialog();
	std::string SaveProjectDialog();
	std::string OpenImageDialog();
	std::string OpenObjDialog();
	std::string OpenVegetationModelDialog();
	std::string PickFolderDialog(const char* title);
}

#endif // !SRC_PLATFORM_NATIVE_DIALOGS_H_


