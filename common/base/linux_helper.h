#pragma once
#include <string>
namespace Astra
{
    class LinuxHelper
    {
    public:
        static void Deamonize();
        static bool ChangeWorkingDirectory(const char *dir);
        static const char *GetBinDirectory();
        static bool LockFile(const char *file);
        static bool MakeDirectoryForFile(const std::string file_name);
        static void SetSignalHandler(int signal_value, void (*handler)(int));
    };
} // namespace Astra