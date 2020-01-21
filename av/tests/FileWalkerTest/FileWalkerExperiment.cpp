//
// Created by pair on 19/12/2019.
//

#include "filewalker/FileWalker.h"
#include <iostream>

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

int main(int argc, char* argv[])
{
    CallbackImpl callbacks;
    for(int i=1; i < argc; i++)
    {
        filewalker::walk(argv[i], callbacks);
    }

    return 0;
}