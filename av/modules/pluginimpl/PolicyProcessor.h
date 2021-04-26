/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/XmlUtilities/AttributesMap.h>

namespace Plugin
{
    class PolicyProcessor
    {
    public:
        PolicyProcessor();
        /**
         *
         * @param policy
         * @return True if the customer ID has changed - if sophos_threat_detector needs to be restarted
         */
        bool processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy);

        /**
         *
         * @param policy
         * @return True if the Enable SXL Lookup setting has changed - if sophos_threat_detector needs to be restarted
         */
        bool processSavPolicy(const Common::XmlUtilities::AttributesMap& policy);

        static std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
        static bool isLookupEnabled(const Common::XmlUtilities::AttributesMap& policy);
        bool getSXL4LookupsEnabled();
        bool m_lookupEnabled = true;

    private:
        std::string m_customerId;
    };
}
