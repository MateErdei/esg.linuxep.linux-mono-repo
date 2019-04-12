/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry/Logger.h"

#include <Common/Logging/FileLoggingSetup.h>

#include <sstream>

namespace Telemetry
{
    int main_entry(int argc, char* argv[])
    {
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("telemetry");

        std::stringstream msg;
        msg << "Running telemetry executable with arguments: ";
        for (int i = 0; i < argc; ++i)
        {
            msg << argv[i] << " ";
        }
        LOGINFO(msg.str());

        return 0;
    }

} // namespace Telemetry
