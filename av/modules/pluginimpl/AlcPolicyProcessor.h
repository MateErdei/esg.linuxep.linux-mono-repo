/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/XmlUtilities/AttributesMap.h>

namespace Plugin
{
    class AlcPolicyProcessor
    {
    public:
        void processAlcPolicy(const Common::XmlUtilities::AttributesMap& map);
    };
}
