/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "diagnose_main.h"
#include "GatherFiles.h"

#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstring>

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
    int diagnose_main::main(int argc, char* argv[])
    {
        if(argc > 2)
        {
            std::cout << "Expecting only one parameter got " << (argc-1) << std::endl;
            return 1;
        }

        std::string outputDir = ".";
        if (argc == 2)
        {
            std::string arg(argv[1]);
            if (arg == "--help")
            {
                std::cout << "Expected Usage: ./sophos_diagnose <path_to_output_directory>" << std::endl;
                return 0;
            }
            outputDir = arg;
        }

        try
        {
            GatherFiles gatherFiles;
            gatherFiles.setInstallDirectory(workOutInstallDirectory());
            std::string destination = gatherFiles.createDiagnoseFolder(outputDir);
            gatherFiles.copyLogFiles(destination);
            gatherFiles.copyMcsConfigFiles(destination);
        }
        catch (std::invalid_argument& e)
        {
            std::cout << e.what() << std::endl;
            return 2;
        }
        return 0;
    }

}