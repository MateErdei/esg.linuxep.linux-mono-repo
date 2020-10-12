/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "PluginUtils.h"
#include "ApplicationPaths.h"
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>

namespace Plugin
{
    bool PluginUtils::isRunningModeXDR(const std::string &flagContent)
    {
        bool isXDR = false;

            try
            {
                nlohmann::json j = nlohmann::json::parse(flagContent);

                if (j.find("xdr.enabled") != j.end())
                {
                    if (j["xdr.enabled"] == true)
                    {
                        isXDR = true;
                    }
                }
            }
            catch (nlohmann::json::parse_error &ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not parse json: " << flagContent << " with error: " << ex.what();
                LOGWARN(errorMessage.str());
            }

        return isXDR;
    }


}