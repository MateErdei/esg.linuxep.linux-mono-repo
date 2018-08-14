/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ManifestDiff.h"
#include "CommandLineOptions.h"

#include <cassert>

using namespace Installer::ManifestDiff;

int ManifestDiff::manifestDiffMain(int argc, char** argv)
{
    Common::Datatypes::StringVector argvv;

    assert(argc>=1);
    for(int i=0; i<argc; i++)
    {
        argvv.emplace_back(argv[i]);
    }

    return manifestDiffMain(argvv);
}

int ManifestDiff::manifestDiffMain(const Common::Datatypes::StringVector& argv)
{
    CommandLineOptions options(argv);
    return 0;
}
