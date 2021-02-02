/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>
#include <Common/XmlUtilities/AttributesMap.h>
#include <thirdparty/nlohmann-json/json.hpp>
#include "Logger.h"

namespace Plugin
{
    std::optional<std::string> getCustomQueries(const std::string &liveQueryPolicy) {
        const std::string customQueries = "policy/configuration/scheduled/customQueries";
        const std::string queryTag = "customQuery";

        Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(liveQueryPolicy);
        Common::XmlUtilities::Attributes attributes = attributesMap.lookup(customQueries + "/" + queryTag);

        if (attributes.empty()) {
            LOGINFO("No custom queries in LiveQuery policy");
            return std::optional<std::string>();
        }

        nlohmann::json customQueryPack;
        int i = 0;
        bool queryAdded = false;

        while (true) {
            std::string suffix = "";

            if (i != 0) {
                suffix = "_" + std::to_string(i - 1);
            }

            i++;
            std::string key = customQueries + "/" + queryTag + suffix;
            Common::XmlUtilities::Attributes customQuery = attributesMap.lookup(key);

            if (customQuery.empty()) {
                break;
            }

            std::string queryName = customQuery.value("queryName", "");
            std::string query = attributesMap.lookup(key + "/query").value("TextId", "");
            std::string interval = attributesMap.lookup(key + "/interval").value("TextId", "");

            if (interval.empty() || query.empty() || queryName.empty()) {
                LOGWARN("Custom query is missing mandatory fields");
                continue;
            }

            std::string tag = attributesMap.lookup(key + "/tag").value("TextId", "");
            std::string description = attributesMap.lookup(key + "/description").value("TextId", "");
            std::string denylist = attributesMap.lookup(key + "/denylist").value("TextId", "");
            std::string removed = attributesMap.lookup(key + "/removed").value("TextId", "");

            const std::string schedule_string{"schedule"};
            customQueryPack[schedule_string][queryName]["query"] = query;
            customQueryPack[schedule_string][queryName]["tag"] = tag;
            customQueryPack[schedule_string][queryName]["interval"] = interval;
            customQueryPack[schedule_string][queryName]["description"] = description;
            customQueryPack[schedule_string][queryName]["denylist"] = denylist;
            customQueryPack[schedule_string][queryName]["removed"] = removed;
            queryAdded = true;
        }

        if (!queryAdded) {
            LOGWARN("No valid queries found in LiveQuery Policy");
            return std::nullopt;
        }

        return customQueryPack.dump();
    }
}