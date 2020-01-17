//
// Created by pair on 19/12/2019.
//

#include "filewalker/FileWalker.h"
#include <iostream>

static void printout(const fs::path& p)
{
    std::cout << p << '\n';

}

int main(int argc, char* argv[])
{
    for(int i=1; i < argc; i++)
    {
        auto fw = filewalker::FileWalker(argv[i], printout);
        fw.run();
    }

    return 0;
}