/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "wdctl_bootstrap.h"

using namespace wdctl::wdctlimpl;

int wdctl_bootstrap::main(int argc, char **argv)
{
    StringVector args = convertArgv(static_cast<unsigned int>(argc), argv);
    return main(args);
}

wdctl_bootstrap::StringVector wdctl_bootstrap::convertArgv(unsigned int argc, char **argv)
{
    StringVector result;
    result.reserve(argc);
    for (unsigned int i=0; i<argc; ++i)
    {
        result.emplace_back(argv[i]);
    }
    return result;
}

int wdctl_bootstrap::main(const wdctl_bootstrap::StringVector& args)
{
    static_cast<void>(args);
    return 0;
}
