/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <modules/osqueryextensions/SophosServerTable.h>

using namespace OsquerySDK;
static int extension_runner_main(int argc, char* argv[])
{

    try
    {
        auto flags = OsquerySDK::ParseFlags(&argc, &argv);

        auto extension = OsquerySDK::CreateExtension(flags, "SophosExtension", "1.0.0");
        // add new extension table here
        extension->AddTablePlugin(std::make_unique<OsquerySDK::SophosServerTable>());

        extension->Start();
        extension->Wait();
        return extension->GetReturnCode();
    }
    catch (const std::exception& err)
    {
        std::cout << "An error occurred while the extension logger was unavailable: " << err.what() << std::endl;

        return 1;
    }

}

MAIN(extension_runner_main(argc, argv))