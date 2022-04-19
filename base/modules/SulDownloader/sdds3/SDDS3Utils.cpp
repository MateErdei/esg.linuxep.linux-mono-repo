/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "SDDS3Repository.h"

#include <Config.h>
#include <PackageRef.h>
#include <json.hpp>

namespace SulDownloader
{

    std::string writeSUSRequest(const SUSRequestParameters& parameters)
    {
        nlohmann::json json;
        json["schema_version"] = 1;
        json["product"] = parameters.product;
        json["server"] = parameters.isServer;
        json["platform_token"] = parameters.platformToken;
        json["subscriptions"] = nlohmann::json::array();
        for (const auto& subscription : parameters.subscriptions)
        {
            std::map<std::string, std::string> subscriptionMap = { { "id", subscription.rigidName() }, { "tag", subscription.tag() }};
            if (!subscription.fixedVersion().empty())
            {
                subscriptionMap.insert({ "fixedVersion", subscription.fixedVersion() });
            }
            json["subscriptions"].push_back(subscriptionMap);
        }

        return json.dump();
    }

} // namespace SulDownloader
