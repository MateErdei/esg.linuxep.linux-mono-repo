// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "modules/common/StoppableSleeper.h"
#include "modules/common/ThreatDetector/SusiSettings.h"
#include "scan_messages/ProcessControlSerialiser.h"

#include <Common/XmlUtilities/AttributesMap.h>
#include <thirdparty/nlohmann-json/json.hpp>

namespace Plugin
{
    enum class PolicyType
    {
        ALC,
        SAV,
        CORC,
        CORE,
        UNKNOWN
    };

    /**
     * The policy is semantically invalid
     */
    class InvalidPolicyException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class PolicyProcessor
    {
    public:
        using IStoppableSleeperSharedPtr = common::IStoppableSleeperSharedPtr;
        explicit PolicyProcessor(IStoppableSleeperSharedPtr stoppableSleeper);
        virtual ~PolicyProcessor() = default;

        /*
         * Returns the type of policy received based on APPID and, where needed, policy type attribute
         * For example there are 2 SAV polices.         *
         */
        static PolicyType determinePolicyType(
            const Common::XmlUtilities::AttributesMap& policy,
            const std::string& appId);

        /**
         *
         * @param policy
         * @return True if the customer ID has changed - if sophos_threat_detector needs to be restarted
         */
        void processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy);

        void processSavPolicy(const Common::XmlUtilities::AttributesMap& policy);

        void processCOREpolicy(const Common::XmlUtilities::AttributesMap& policy);

        void processCorcPolicy(const Common::XmlUtilities::AttributesMap& policy);

        static std::string getCustomerId(const Common::XmlUtilities::AttributesMap& policy);
        static bool isLookupEnabled(const Common::XmlUtilities::AttributesMap& policy);
        [[nodiscard]] bool getSXL4LookupsEnabled() const;

        static void setOnAccessConfiguredTelemetry(bool enabled);

        void processFlagSettings(const std::string& flagsJson);
        [[nodiscard]] bool isSafeStoreEnabled() const;

        [[nodiscard]] bool restartThreatDetector() const
        {
            return m_restartThreatDetector;
        }

    protected:
        virtual void notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE requestType);

        void processOnAccessSettingsFromCOREpolicy(const Common::XmlUtilities::AttributesMap& policy);

    private:
        void processOnAccessFlagSettings(const nlohmann::json& flagsJson);
        void processSafeStoreFlagSettings(const nlohmann::json& flagsJson);
        void saveSusiSettings();

        static std::vector<std::string> extractListFromXML(
            const Common::XmlUtilities::AttributesMap& policy,
            const std::string& entityFullPath);

        IStoppableSleeperSharedPtr m_sleeper;
        std::string m_customerId;
        common::ThreatDetector::SusiSettings m_threatDetectorSettings;
        bool m_safeStoreEnabled = false;

        bool m_gotFirstSavPolicy = false;
        bool m_gotFirstAlcPolicy = false;
        bool m_gotFirstCorcPolicy = false;
        bool m_restartThreatDetector = false;

        inline static const std::string OA_FLAG { "av.onaccess.enabled" };
        inline static const std::string SS_FLAG { "safestore.enabled" };
    };
} // namespace Plugin
