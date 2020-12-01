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
        void processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy);
    private:
        std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
    };
}
