/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/XmlUtilities/AttributesMap.h>

#ifndef SLEEP_TIME
# define SLEEP_TIME 1
#endif

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
        bool processSavPolicy(const Common::XmlUtilities::AttributesMap& policy, bool isSAVPolicyAlreadyProcessed = true);

        static std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
        static bool isLookupEnabled(const Common::XmlUtilities::AttributesMap& policy);
        [[nodiscard]] bool getSXL4LookupsEnabled() const;

    private:
        std::string m_customerId;
        bool m_lookupEnabled = true;
        static bool isOnAccessEnabled(const Common::XmlUtilities::AttributesMap& policy);
        static void processOnAccessPolicy(const Common::XmlUtilities::AttributesMap& policy,
                                          const struct timespec& socketSleepTime= { SLEEP_TIME, 0 });
        static std::vector<std::string> extractListFromXML(
            const Common::XmlUtilities::AttributesMap& policy,
            const std::string& entityFullPath);
    };
}
