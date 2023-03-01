// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include <json.hpp>

namespace watchdog::watchdogimpl
{
    using WatchdogUserGroupIDs = nlohmann::json;
    std::string stripCommentsFromRequestedUserGroupIdFile();
    WatchdogUserGroupIDs readRequestedUserGroupIds();
}
