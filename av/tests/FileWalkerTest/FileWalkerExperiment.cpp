//
// Created by pair on 19/12/2019.
//

#include "filewalker/FileWalker.h"

int main(int argc, char* argv[])
{
    for(int i=1; i < argc; i++)
    {
        auto fw = filewalker::FileWalker(argv[i]);
        fw.run();
    }

    return 0;
}