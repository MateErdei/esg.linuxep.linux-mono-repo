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
    return false;
#endif
}
