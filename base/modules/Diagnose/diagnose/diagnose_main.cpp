/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "diagnose_main.h"
#include "GatherFiles.h"

#include <stdlib.h>
#include <string>

namespace
{
    std::string work_out_install_directory()
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
    int diagnose_main::main(int argc)
    {
        std::string installDirectory = work_out_install_directory();
        GatherFiles gatherFiles;
        gatherFiles.copyLogFiles(installDirectory);
        gatherFiles.copyMcsConfigFiles(installDirectory);

        if (argc>1){return 1;}

        return 1;
    }

}