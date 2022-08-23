/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ProcessControlSerialiser.h"

#include <Common/XmlUtilities/AttributesMap.h>

namespace Plugin
{
    class PolicyProcessor
    {
    public:
        PolicyProcessor();
        virtual ~PolicyProcessor() = default;

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

        void processOnAccessPolicy(const Common::XmlUtilities::AttributesMap& policy);

        static std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
        static bool isLookupEnabled(const Common::XmlUtilities::AttributesMap& policy);
        [[nodiscard]] bool getSXL4LookupsEnabled() const;

        static void setOnAccessConfiguredTelemetry(bool enabled);

        void processFlagSettings(const std::string& flagsJson);

    protected:
        virtual void notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE requestType);

    private:
        std::string m_customerId;
        bool m_lookupEnabled = true;
        static std::vector<std::string> extractListFromXML(
            const Common::XmlUtilities::AttributesMap& policy,
            const std::string& entityFullPath);

        inline static const std::string OA_FLAG = "av.oa_enabled";
    };
}
