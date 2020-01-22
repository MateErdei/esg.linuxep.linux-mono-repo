/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/XmlUtilities/AttributesMap.h>

#include <string>

namespace manager::scheduler
{
    class ScheduledScanConfiguration
    {
    public:
        explicit ScheduledScanConfiguration(Common::XmlUtilities::AttributesMap& savPolicy);
    };
}
