// Copyright 2023 Sophos Limited. All rights reserved.

#include "responseaction_main.h"

#include "Logger.h"
#include "RunUtils.h"
#include "config.h"

#include <Common/Logging/PluginLoggingSetup.h>

#include <iostream>
namespace ActionRunner
{

    int responseaction_main::main(int argc, char* argv[])
    {
        if (argc != 4)
        {
            std::cerr << "Expecting three parameters got " << (argc - 1) << std::endl;
            return 1;
        }

        Common::Logging::PluginLoggingSetup loggerSetup(RA_PLUGIN_NAME, "actionrunner");
        std::string correlationId = argv[1];
        std::string action = argv[2];
        std::string type = argv[3];
        int returnCode = 0;
        if (type == "sophos.mgt.action.UploadFile")
        {
            LOGINFO("Running upload action: " << correlationId);
            std::string response = RunUtils::doUpload(action);
            RunUtils::sendResponse(correlationId, response);
            LOGINFO("Sent upload response for id " << correlationId << " to Central");
        }
        else if (type == "sophos.mgt.action.UploadFolder")
        {
            LOGINFO("Running upload folder action: " << correlationId);
            std::string response = RunUtils::doUploadFolder(action);
            RunUtils::sendResponse(correlationId, response);
            LOGINFO("Sent upload folder response for id " << correlationId << " to Central");
        }
        else
        {
            LOGWARN("Throwing away unknown action: " << action);
        }

        return returnCode;
    }

} // namespace ActionRunner
