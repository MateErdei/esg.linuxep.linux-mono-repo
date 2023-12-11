// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SDDS3Utils.h"

#include "Logger.h"

#include "sophlib/sdds3/Config.h"

#include <nlohmann/json.hpp>

namespace SulDownloader
{

    std::string createSUSRequestBody(const SUSRequestParameters& parameters)
    {
        nlohmann::json json;
        json["schema_version"] = 1;
        json["product"] = parameters.product;
        json["server"] = parameters.isServer;
        json["platform_token"] = parameters.platformToken;
        json["subscriptions"] = nlohmann::json::array();

        for (const auto& subscription : parameters.subscriptions)
        {
            std::map<std::string, std::string> subscriptionMap = { { "id", subscription.rigidName() },
                                                                   { "tag", subscription.tag() } };
            if (!subscription.fixedVersion().empty())
            {
                subscriptionMap.insert({ "fixedVersion", subscription.fixedVersion() });
            }
            json["subscriptions"].push_back(subscriptionMap);
        }

        if (parameters.esmVersion.isEnabled())
        {
            json["fixed_version_token"] = parameters.esmVersion.token();
        }

        return json.dump();
    }

} // namespace SulDownloader
