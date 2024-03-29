
#include "ReactorImplTestsPath.h"

#include <gtest/gtest.h>

#include <cstdio>

GTEST_API_ int main(int argc, char** argv)
{
    printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);
    setProgramPath(argv[0]);
    return RUN_ALL_TESTS();
}