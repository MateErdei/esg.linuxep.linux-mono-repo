/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LiveQueryPolicyParser.h"

#include "Logger.h"
#include "PluginUtils.h"

#include <json/json.h>
#include <thirdparty/nlohmann-json/json.hpp>
#include <modules/pluginimpl/ApplicationPaths.h>

#include <string>

namespace Plugin
{
    unsigned int getDataLimit(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap)
    {
        int policyDataLimitAsInt = 262144000;
        try
        {
            const std::string dataLimitPath{"policy/configuration/scheduled/dailyDataLimit"};
            Common::XmlUtilities::Attributes attributes = liveQueryPolicyMap.value().lookup(dataLimitPath);
            std::string policyDataLimitAsString = attributes.contents();
            policyDataLimitAsInt = std::stoi(policyDataLimitAsString);
            LOGDEBUG("Using dailyDataLimit from LiveQuery Policy: " << policyDataLimitAsInt);
        }
        catch (const std::exception& e)
        {
            LOGERROR("Failed to extract dailyDataLimit from LiveQuery Policy: " << e.what() << ". Using default 262144000");
        }
        return policyDataLimitAsInt;
    }

    std::string getRevId(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap)
    {
        std::string revId = "";
        try
        {
            const std::string dataLimitPath{"policy"};
            Common::XmlUtilities::Attributes attributes = liveQueryPolicyMap.value().lookup(dataLimitPath);
            revId = attributes.value("RevID", "");
        }
        catch (const std::exception& e)
        {
            LOGERROR("Failed to extract revId from LiveQuery Policy: " << e.what());
        }

        if (revId != "")
        {
            LOGDEBUG("Got RevID: " << revId << " from LiveQuery Policy");
        }
        else {
            throw FailedToParseLiveQueryPolicy("Didn't find RevID");
        }
        return revId;
    }

    std::optional<std::string> getCustomQueries(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap)
    {
        bool queryAdded = false;
        nlohmann::json customQueryPack;

        try
        {
            const std::string customQueries = "policy/configuration/scheduled/customQueries";
            const std::string queryTag = "customQuery";

            Common::XmlUtilities::Attributes attributes = liveQueryPolicyMap.value().lookup(
                    customQueries + "/" + queryTag);

            if (attributes.empty()) {
                LOGINFO("No custom queries in LiveQuery policy");
                return std::nullopt;
            }

            int i = 0;

            while (true) {
                std::string suffix = "";

                if (i != 0) {
                    suffix = "_" + std::to_string(i - 1);
                }

                i++;
                std::string key = customQueries + "/" + queryTag + suffix;
                Common::XmlUtilities::Attributes customQuery = liveQueryPolicyMap.value().lookup(key);

                if (customQuery.empty()) {
                    break;
                }

                std::string queryName = customQuery.value("queryName", "");
                std::string query = liveQueryPolicyMap.value().lookup(key + "/query").value("TextId", "");
                std::string interval = liveQueryPolicyMap.value().lookup(key + "/interval").value("TextId", "");

                if (interval.empty() || query.empty() || queryName.empty()) {
                    LOGWARN("Custom query is missing mandatory fields");
                    continue;
                }

                std::string tag = liveQueryPolicyMap.value().lookup(key + "/tag").value("TextId", "");
                std::string description = liveQueryPolicyMap.value().lookup(key + "/description").value("TextId",
                                                                                                        "");
                std::string denylist = liveQueryPolicyMap.value().lookup(key + "/denylist").value("TextId", "");
                std::string removed = liveQueryPolicyMap.value().lookup(key + "/removed").value("TextId", "");

                const std::string schedule_string{"schedule"};
                customQueryPack[schedule_string][queryName]["query"] = query;
                customQueryPack[schedule_string][queryName]["tag"] = tag;
                customQueryPack[schedule_string][queryName]["interval"] = interval;
                customQueryPack[schedule_string][queryName]["description"] = description;
                customQueryPack[schedule_string][queryName]["denylist"] = denylist;
                customQueryPack[schedule_string][queryName]["removed"] = removed;
                queryAdded = true;
            }
        }
        catch (const std::exception &e)
        {
            LOGERROR("Failed to extract custom queries from LiveQuery Policy: " << e.what());
            return std::nullopt;
        }

        if (!queryAdded) {
            LOGWARN("No valid queries found in LiveQuery Policy");
            return std::nullopt;
        }

        return customQueryPack.dump();
    }

    bool getFoldingRules(const std::optional<Common::XmlUtilities::AttributesMap>& liveQueryPolicyMap, std::vector<Json::Value>& foldingRules)
    {
        bool hasBadRule = false;
        Json::CharReaderBuilder builder;
        auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
        Json::Value root;
        std::string errors;

        try
        {
            const std::string foldingRulesPath = "policy/configuration/scheduled/foldingRules";
            Common::XmlUtilities::Attributes attributes = liveQueryPolicyMap.value().lookup(foldingRulesPath);

            if (attributes.empty())
            {
                LOGDEBUG("No folding rules in LiveQuery policy");
                foldingRules = std::vector<Json::Value> {};
                return true;
            }

            reader->parse(
                    attributes.contents().c_str(),
                    attributes.contents().c_str() + attributes.contents().size(),
                    &root,
                    &errors);
        }
        catch (const std::exception &e)
        {
            LOGERROR("Failed to extract folding rules from LiveQuery Policy: " << e.what());
            foldingRules = std::vector<Json::Value> {};
            return false;
        }

        if (!errors.empty())
        {
            LOGWARN("Unable to parse folding rules:" << errors);
            foldingRules = std::vector<Json::Value> {};
            return false;
        }

        try
        {
            for (Json::Value::const_iterator it = root.begin(); it != root.end(); ++it)
            {
                if (!it->isObject())
                {
                    LOGWARN("Unexpected type " << it->type() << " for folding rule");
                    hasBadRule = true;
                    continue;
                }
                if (!it->isMember("query_name") || !it->isMember("values"))
                {
                    LOGWARN("Invalid folding rule");
                    hasBadRule = true;
                    continue;
                }

                foldingRules.push_back(*it);
            }
        }
        catch (const std::exception& err)
        {
            LOGWARN("Failed to process folding rules. Error:" << err.what());
            foldingRules = std::vector<Json::Value> {};
            return false;
        }

        if (foldingRules.empty() && hasBadRule)
        {
            return false;
        }
        return true;
    }

    std::map<std::string, std::string> getLiveQueryPackIdToConfigPath()
    {
        return {{"XDR", Plugin::osqueryXDRConfigFilePath()}, {"MTR", Plugin::osqueryMTRConfigFilePath()}};
    }

    //    <?xml version="1.0"?>
//    <policy type="LiveQuery" RevID="{{revId}}" policyType="56">
//      <configuration>
//          <scheduled>
//              <enabled>true/false<enabled/>
//              <dailyDataLimit>{{dailyDataLimit}}</dailyDataLimit>
//              <queryPacks>
//                  <queryPack id="{{queryPackId}}" />
//                  ...
//                  XDR
//                  MTR
//              </queryPacks>
//          </scheduled>
//      </configuration>
//    </policy>
    std::vector<std::string> getEnabledQueryPacksInPolicy(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap)
    {
        const std::string queryPacks = "policy/configuration/scheduled/queryPacks";
        const std::string queryPackAttributeName = "queryPack";
        const std::string scheduledQueriesEnabledXmlPath = "policy/configuration/scheduled/enabled";
        std::vector<std::string> enabledQueryPacks;

        try
        {
            std::vector<Common::XmlUtilities::Attributes> attributes = liveQueryPolicyMap.value().lookupMultiple(queryPacks + "/" + queryPackAttributeName);
            for (auto queryPack: attributes)
            {
                std::string queryPackIdString = queryPack.value("id", "");
                if (queryPackIdString.empty())
                {
                    LOGDEBUG("queryPack entry is missing mandatory fields");
                    continue;
                }
                LOGDEBUG("Policy lists: " << queryPackIdString << ", to be enabled");

                if (getLiveQueryPackIdToConfigPath().count(queryPackIdString) == 1)
                {
                    enabledQueryPacks.push_back(queryPackIdString);
                }
                else
                {
                    LOGDEBUG("Query pack: " << queryPackIdString << " is not an understood query pack ID");
                }
            }
        }
        catch (const std::exception &e)
        {
            LOGERROR("Failed to extract query packs from LiveQuery Policy: " << e.what());
        }

        return enabledQueryPacks;
    }

    bool getScheduledQueriesEnabledInPolicy(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap)
    {
        bool scheduledQueriesEnabled = false;
        const std::string scheduledQueriesEnabledXmlPath = "policy/configuration/scheduled/enabled";

        try
        {
            Common::XmlUtilities::Attributes scheduledQueriesEnabledAttribute = liveQueryPolicyMap.value().lookup(scheduledQueriesEnabledXmlPath);
            std::string scheduledQueriesEnabledString = scheduledQueriesEnabledAttribute.value("TextId", "false");
            scheduledQueriesEnabled = (scheduledQueriesEnabledString == "true");
        }
        catch (const std::exception &e)
        {
            LOGERROR("Failed to extract enabled option from LiveQuery Policy: " << e.what());
        }
        return scheduledQueriesEnabled;
    }


} // namespace Plugin