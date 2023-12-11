// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/FlagUtils/FlagUtils.h"

#include "Common/FlagUtils/Logger.h"

#include <nlohmann/json.hpp>
#include <sstream>

namespace Common
{
    bool FlagUtils::isFlagSet(const std::string& flag, const std::string& flagContent)
    {
        bool flagValue = false;

        try
        {
            nlohmann::json j = nlohmann::json::parse(flagContent);

            if (j.find(flag) != j.end())
            {
                if (j[flag] == true)
                {
                    flagValue = true;
                }
            }
        }
        catch (nlohmann::json::parse_error& ex)
        {
            std::stringstream errorMessage;
            errorMessage << "Could not parse json: " << flagContent << " with error: " << ex.what();
            LOGWARN(errorMessage.str());
        }

        return flagValue;
    }
} // namespace Common