/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <datatypes/Print.h>
#include <filewalker/FileWalker.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <cassert>
#include <iostream>
#include <string>
#include <fstream>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace fs = sophos_filesystem;

class CallbackImpl : public filewalker::IFileWalkCallbacks
{
public:
    CallbackImpl() = default;
    void processFile(const sophos_filesystem::path& p, bool /*symlinkTarget*/) override
    {
        // Check it's actually a file
        struct stat statbuf{};
        int ret = ::stat(p.c_str(), &statbuf);
        if (ret != 0)
        {
            PRINT("Failed to stat " << p.c_str());
            throw std::runtime_error("Failed to stat");
        }
        if (! S_ISREG(statbuf.st_mode))
        {
            PRINT(p << " is not regular file");
            throw std::runtime_error("Not regular file");
        }

        std::cout << p << '\n';
    }
    bool includeDirectory(const sophos_filesystem::path&) override
    {
        return true;
    }
};

#define BASE "/tmp/TestFileWalkerBackTrackProtection"

int main()
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

    // Create test tree
    fs::remove_all(BASE);
    fs::create_directories(BASE "/a/b");
    std::ofstream(BASE "/a/b/c");

    fs::create_symlink("b", BASE "/a/d");
    fs::create_symlink(BASE "/a/b", BASE "/a/b/e");
    fs::create_symlink("c", BASE "/a/b/f");

    CallbackImpl callbacks;
    filewalker::FileWalker w(callbacks);
    w.followSymlinks();
    w.walk(BASE);

    return 0;
}
