/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TelemetryValue.h"

#include <map>

namespace Common::Telemetry
{
    /*
     * Converts the top level of the given json string into a map
     * Any nested structures or arrays are ignored and no warning is issued.
     */
    std::map<std::string, TelemetryValue> flatJsonToMap(const std::string& jsonString);
}

