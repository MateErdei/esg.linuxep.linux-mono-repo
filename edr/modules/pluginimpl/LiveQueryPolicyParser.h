/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IOsqueryProcess.h"
#include "OsqueryConfigurator.h"
#include "OsqueryDataManager.h"
#include "PluginCallback.h"
#include "QueueTask.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/Process/IProcess.h>
#include <modules/osqueryextensions/LoggerExtension.h>
#include <modules/osqueryextensions/SophosExtension.h>
#include <queryrunner/IQueryRunner.h>
#include <queryrunner/ParallelQueryProcessor.h>

#include <future>
#include <list>

namespace Plugin
{
    std::optional<std::string> getCustomQueries(const std::string& liveQueryPolicy);
    std::vector<Json::Value> getFoldingRules(const std::string& liveQueryPolicy, const std::vector<Json::Value> lastGoodRules);
} // namespace Plugin
