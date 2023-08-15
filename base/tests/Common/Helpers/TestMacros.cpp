// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "TestMacros.h"
#include <unistd.h>

bool skipTest()
{
#ifndef NDEBUG
    std::cout << "not compatible with coverage ... skip" << std::endl;
    return true;
#else
    if (getuid() != 0)
    {
        std::cout << "requires root ... skip" << std::endl;
        return true;
    }
    return false;
#endif

}

bool skipIfCoverage()
{
// I couldnt' find a way to detect that coverage is enabled (gcov), hence assuming debug is the same as coverage
#ifndef NDEBUG
    std::cout << "not compatible with coverage ... skip" << std::endl;
    return true;
#else
    auto covfilePathPtr = std::getenv("COVFILE");
    if(covfilePathPtr != nullptr)
    {
        std::cout << "not compatible with bullseye coverage ... skip" << std::endl;
        return true;
    }
    return false;
#endif
}
