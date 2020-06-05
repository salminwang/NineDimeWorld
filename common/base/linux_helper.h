#pragma once

namespace Astra {

class LinuxHelper {
public:
    static void Deamonize();
    static bool ChangeWorkingDirectory(const char* dir);
    static const char* GetBinDirectory();
    static bool LockFile(const char* file);
};
}