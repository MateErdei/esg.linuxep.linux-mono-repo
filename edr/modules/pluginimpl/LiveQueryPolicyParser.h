// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IOsqueryProcess.h"
#include "OsqueryConfigurator.h"
#include "OsqueryDataManager.h"
#include "PluginCallback.h"
#include "QueueTask.h"

#include "osqueryextensions/LoggerExtension.h"
#include "osqueryextensions/SophosExtension.h"
#include "queryrunner/IQueryRunner.h"
#include "queryrunner/ParallelQueryProcessor.h"

#include "Common/XmlUtilities/AttributesMap.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/Process/IProcess.h"

#include <future>
#include <list>

namespace Plugin
{
    class FailedToParseLiveQueryPolicy : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    unsigned long long int getDataLimit(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap);
    std::string getRevId(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap);
    bool getCustomQueries(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap, std::optional<std::string>& customQueries);
    bool getFoldingRules(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap, std::vector<Json::Value>& foldingRules);
    std::map<std::string, std::string> getLiveQueryPackIdToConfigPath();
    std::vector<std::string> getEnabledQueryPacksInPolicy(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap);
    bool getScheduledQueriesEnabledInPolicy(const std::optional<Common::XmlUtilities::AttributesMap> &liveQueryPolicyMap);


} // namespace Plugin
