/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <datatypes/Print.h>
#include <filewalker/FileWalker.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <cassert>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace fs = sophos_filesystem;

class CallbackImpl : public filewalker::IFileWalkCallbacks
{
public:
    CallbackImpl() = default;
    void processFile(const sophos_filesystem::path& p) override
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
};

#define BASE "/tmp/TestFileWalkerBackTrackProtection"

int main()
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

    // Create test tree
    ::mkdir(BASE, 0700);
    ::mkdir(BASE "/a", 0700);
    ::mkdir(BASE "/a/b", 0700);
    int fd = ::open(BASE "/a/b/c",O_CREAT|O_RDWR, 0700);
    const std::string CONTENTS = "EICAR";
    int ret = ::write(fd, CONTENTS.c_str(), CONTENTS.size());
    static_cast<void>(ret);
    ::close(fd);
    ret = symlink("b", BASE "/a/d");
    assert(ret == 0 || errno == EEXIST);
    static_cast<void>(ret);
    ret = symlink(BASE "/a/b", BASE "/a/b/e");
    assert(ret == 0 || errno == EEXIST);
    static_cast<void>(ret);
    ret = symlink("c", BASE "/a/b/f");
    assert(ret == 0 || errno == EEXIST);
    static_cast<void>(ret);

    CallbackImpl callbacks;
    filewalker::FileWalker w(callbacks);
    w.followSymlinks();
    w.walk(BASE);

    return 0;
}
