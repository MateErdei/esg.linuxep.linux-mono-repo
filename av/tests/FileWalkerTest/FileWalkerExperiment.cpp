//
// Created by pair on 19/12/2019.
//

#include "Common/Logging/ConsoleLoggingSetup.h"

#include "filewalker/FileWalker.h"
#include <iostream>

namespace fs = sophos_filesystem;

class CallbackImpl : public filewalker::IFileWalkCallbacks
{
public:
    CallbackImpl() = default;
    void processFile(const sophos_filesystem::path& p, bool /*symlinkTarget*/) override
    {
        std::cout << "FILE:" << p << '\n';
    }
    bool includeDirectory(const sophos_filesystem::path& p) override
    {
        std::cout << "DIR:" << p << '\n';
        return true;
    }
    bool userDefinedExclusionCheck(const sophos_filesystem::path &p) override
    {
        std::cout << "DIR:" << p << '\n';
        return false;
    }

};

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    CallbackImpl callbacks;
    for(int i=1; i < argc; i++)
    {
        filewalker::walk(argv[i], callbacks);
    }

    return 0;
}