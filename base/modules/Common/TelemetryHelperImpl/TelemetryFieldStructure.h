// Copyright 2023 Sophos All rights reserved.

//This file defines fields which need to be moved to the top level of telemetry
//First field is where telemetry is logged initially
//Second field is the field name we move to

#pragma once

#include <map>
#include <string>

namespace Common::Telemetry
{
    const std::map<std::string, std::string> fieldsToMoveToTopLevel{ std::make_pair("updatescheduler.esmName", "esmName"),
                                                                     std::make_pair("updatescheduler.esmToken", "esmToken"),
                                                                     std::make_pair("updatescheduler.suite-version", "suiteVersion"),
                                                                     std::make_pair("base-telemetry.tenantId", "tenantId"),
                                                                     std::make_pair("base-telemetry.deviceId", "deviceId") };

}
