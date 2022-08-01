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
    bool userDefinedExclusionCheck(const sophos_filesystem::path& p, bool /*isSymlink*/) override
    {
        std::cout << "DIR:" << p << '\n';
        return false;
    }
    void registerError(const std::ostringstream &errorString, std::error_code ec) override
    {
        std::cerr << errorString.str() << ec << '\n';
    }

};

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    CallbackImpl callbacks;
    filewalker::FileWalker walker(callbacks);
    walker.abortOnMissingStartingPoint(false);
    walker.stayOnDevice(true);

    for(int i=1; i < argc; i++)
    {
        walker.walk(argv[i]);
    }

    return 0;
}