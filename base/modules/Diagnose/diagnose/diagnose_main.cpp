/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "diagnose_main.h"
#include "GatherFiles.h"

#include <stdlib.h>
#include <string>

namespace
{
    std::string workOutInstallDirectory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }

        // If we can't get the cwd then use a fixed string.
        return "/opt/sophos-spl";
    }
} // namespace
namespace diagnose
{
    int diagnose_main::main()
    {
        GatherFiles gatherFiles;
        gatherFiles.setInstallDirectory(workOutInstallDirectory());
        std::string destination = gatherFiles.createDiagnoseFolder("/tmp/temp/");
        gatherFiles.copyLogFiles(destination,"/tmp/log");
        gatherFiles.copyMcsConfigFiles(destination, "/tmp/mcs");
        return 0;
    }

}