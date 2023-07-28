// Copyright 2023 Sophos All rights reserved.

//This file defines fields which need to be moved to the top level of telemetry
//First field is where telemetry is logged initially
//Second field is the field name we move

#pragma once

#include <map>
#include <string>

namespace Telemetry
{
    const std::map<std::string, std::string> fieldsToMoveToTopLevel{ std::make_pair("updatescheduler", "esmName"),
                                                                     std::make_pair("updatescheduler", "esmToken"),
                                                                     std::make_pair("updatescheduler", "suiteVersion"),
                                                                     std::make_pair("system-telemetry", "tenantId"),
                                                                     std::make_pair("system-telemetry", "deviceId") };
}
