/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <cassert>
#include <filewalker/FileWalker.h>

namespace fs = sophos_filesystem;

class CallbackImpl : public filewalker::IFileWalkCallbacks
{
public:
    CallbackImpl() = default;
    void processFile(const sophos_filesystem::path& p)
    {
        std::cout << p << '\n';
    }
};

#define BASE "/tmp/TestFileWalkerBackTrackProtection"

int main()
{
    // Create test tree
    ::mkdir(BASE, 0700);
    ::mkdir(BASE "/a", 0700);
    ::mkdir(BASE "/a/b", 0700);
    int fd = ::open(BASE "/a/b/c",O_CREAT|O_RDWR, 0700);
    const std::string CONTENTS = "EICAR";
    ::write(fd, CONTENTS.c_str(), CONTENTS.size());
    ::close(fd);
    int ret = symlink("b", BASE "/a/d");
    assert(ret == 0);
    ret = symlink(BASE "/a/b", BASE "/a/b/e");
    assert(ret == 0);


    CallbackImpl callbacks;
    filewalker::walk(BASE, callbacks);

    return 0;
}
