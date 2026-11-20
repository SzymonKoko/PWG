#include <glad/glad.h>
#include "core/application/Application.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
// Ask Optimus / PowerXpress laptops to run this process on the discrete GPU (DEC-13 Q3). The Windows per-exe setting
// and the renderer check of the experiment runner (expect_renderer) stay in place.
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

int main(int argc, char** argv)
{
    pwg::ApplicationOptions options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--experiment" && i + 1 < argc)
        {
            options.experimentConfig = argv[++i];
        }
        else if (argument == "--out" && i + 1 < argc)
        {
            options.outputFolder = argv[++i];
        }
        else if (argument == "--exit-when-done")
        {
            options.exitWhenDone = true;
        }
        else
        {
            std::fprintf(stderr, "Usage: PWG.exe [--experiment <config.yaml> [--out <folder>] [--exit-when-done]]\n");
            return 2;
        }
    }
    if (options.experimentConfig.empty() && (!options.outputFolder.empty() || options.exitWhenDone))
    {
        std::fprintf(stderr, "--out and --exit-when-done need --experiment\n");
        return 2;
    }

    pwg::Application app(options);
    app.Run();

    return app.GetExitCode();
}
