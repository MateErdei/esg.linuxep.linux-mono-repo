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
        AlcPolicyProcessor();
        /**
         *
         * @param policy
         * @return True if the customer ID has changed - if sophos_threat_detector needs to be restarted
         */
        bool processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy);

        static std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
    private:
        std::string m_customerId;
    };
}
