// Copyright 2023 Sophos Limited. All rights reserved.

#include "responseaction_main.h"

#include "Logger.h"
#include "RunUtils.h"
#include "config.h"

#include "ResponseActions/RACommon/ResponseActionsCommon.h"

#include "Common/Logging/PluginLoggingSetup.h"

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
        if (type == ResponseActions::RACommon::UPLOAD_FILE_REQUEST_TYPE)
        {
            LOGINFO("Running upload action: " << correlationId);
            std::string response = RunUtils::doUpload(action);
            ResponseActions::RACommon::sendResponse(correlationId, response);
            LOGINFO("Sent upload response for ID " << correlationId << " to Central");
        }
        else if (type == ResponseActions::RACommon::UPLOAD_FOLDER_REQUEST_TYPE)
        {
            LOGINFO("Running upload folder action: " << correlationId);
            std::string response = RunUtils::doUploadFolder(action);
            ResponseActions::RACommon::sendResponse(correlationId, response);
            LOGINFO("Sent upload folder response for ID " << correlationId << " to Central");
        }
        else if (type == ResponseActions::RACommon::RUN_COMMAND_REQUEST_TYPE)
        {
            LOGINFO("Performing run command action: " << correlationId);
            LOGDEBUG(action);
            auto response = RunUtils::doRunCommand(action, correlationId);
            ResponseActions::RACommon::sendResponse(correlationId, response);
            LOGINFO("Sent run command response for ID " << correlationId << " to Central");
        }
        else if (type == "sophos.mgt.action.DownloadFile")
        {
            LOGINFO("Running download file action: " << correlationId);
            std::string response = RunUtils::doDownloadFile(action);
            ResponseActions::RACommon::sendResponse(correlationId, response);
            LOGINFO("Sent download file response for id " << correlationId << " to Central");
        }
        else
        {
            LOGWARN("Throwing away unknown action: " << action);
        }

        return returnCode;
    }

} // namespace ActionRunner
