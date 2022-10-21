// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "modules/common/IStoppableSleeper.h"
#include "scan_messages/ProcessControlSerialiser.h"

#include <Common/XmlUtilities/AttributesMap.h>
#include <thirdparty/nlohmann-json/json.hpp>

namespace Plugin
{
    class PolicyProcessor
    {
    public:
        using IStoppableSleeperSharedPtr = common::IStoppableSleeperSharedPtr;
        explicit PolicyProcessor(IStoppableSleeperSharedPtr stoppableSleeper);
        virtual ~PolicyProcessor() = default;

        /**
         *
         * @param policy
         * @return True if the customer ID has changed - if sophos_threat_detector needs to be restarted
         */
        void processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy);

        void processSavPolicy(const Common::XmlUtilities::AttributesMap& policy);

        void processOnAccessPolicy(const Common::XmlUtilities::AttributesMap& policy);

        static std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
        static bool isLookupEnabled(const Common::XmlUtilities::AttributesMap& policy);
        [[nodiscard]] bool getSXL4LookupsEnabled() const;

        static void setOnAccessConfiguredTelemetry(bool enabled);

        void processFlagSettings(const std::string& flagsJson);
        [[nodiscard]] bool isSafeStoreEnabled() const;

        [[nodiscard]] bool restartThreatDetector() const { return m_restartThreatDetector; }

    protected:
        virtual void notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE requestType);

    private:
        void processOnAccessFlagSettings(const nlohmann::json& flagsJson);
        void processSafeStoreFlagSettings(const nlohmann::json& flagsJson);

        static std::vector<std::string> extractListFromXML(
            const Common::XmlUtilities::AttributesMap& policy,
            const std::string& entityFullPath);

        IStoppableSleeperSharedPtr m_sleeper;
        std::string m_customerId;
        bool m_lookupEnabled = true;
        bool m_safeStoreEnabled = false;

        bool m_gotFirstSavPolicy = false;
        bool m_gotFirstAlcPolicy = false;
        bool m_restartThreatDetector = false;

        inline static const std::string OA_FLAG{"av.onaccess.enabled"};
        inline static const std::string SS_FLAG{"safestore.enabled"};
    };
}
