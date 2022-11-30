// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "modules/common/StoppableSleeper.h"
#include "modules/common/ThreatDetector/SusiSettings.h"
#include "scan_messages/ProcessControlSerialiser.h"

#include <Common/XmlUtilities/AttributesMap.h>
#include <thirdparty/nlohmann-json/json.hpp>

#define USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

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
        using AttributesMap = Common::XmlUtilities::AttributesMap;
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
        void processAlcPolicy(const AttributesMap& policy);

        void processSavPolicy(const AttributesMap& policy);

        void processCOREpolicy(const AttributesMap& policy);

        void processCorcPolicy(const Common::XmlUtilities::AttributesMap& policy);

        static std::string getCustomerId(const AttributesMap& policy);
        static bool isLookupEnabled(const AttributesMap& policy);
        [[nodiscard]] bool getSXL4LookupsEnabled() const;

        static void setOnAccessConfiguredTelemetry(bool enabled);

        void processFlagSettings(const std::string& flagsJson);
        [[nodiscard]] bool isSafeStoreEnabled() const;

        [[nodiscard]] bool restartThreatDetector() const
        {
            return m_restartThreatDetector;
        }

        /**
         * Try and send a reload request to SOAPD if we can
         */
        virtual void notifyOnAccessProcessIfRequired();

        using clock_t = std::chrono::steady_clock;
        using seconds_t = std::chrono::seconds;
        using timepoint_t = std::chrono::time_point<clock_t>;

        /**
         * Get the timeout before we should notify SOAPD
         * @return
         */
        [[nodiscard]] timepoint_t timeout() const;

    TEST_PUBLIC:
        [[nodiscard]] timepoint_t timeout(timepoint_t now) const;

    protected:
        void markOnAccessReloadPending();

        void processOnAccessSettingsFromCOREpolicy(const AttributesMap& policy);
        void processOnAccessSettingsFromSAVpolicy(const AttributesMap& policy);

    private:
        void processOnAccessFlagSettings(const nlohmann::json& flagsJson);
        void processSafeStoreFlagSettings(const nlohmann::json& flagsJson);
        void saveSusiSettings();

        static std::vector<std::string> extractListFromXML(
            const AttributesMap& policy,
            const std::string& entityFullPath);

        static nlohmann::json readOnAccessConfig();
        void writeOnAccessConfig(const nlohmann::json& configJson);

        IStoppableSleeperSharedPtr m_sleeper;
        std::string m_customerId;
        common::ThreatDetector::SusiSettings m_threatDetectorSettings;
        bool m_safeStoreEnabled = false;

        /**
         * Live Protection SXL4 Global Reputation look ups are enabled / disabled in policy
         */
        bool m_lookupEnabled = false;

        bool m_gotFirstSavPolicy = false;
        bool m_gotFirstAlcPolicy = false;
        bool m_gotFirstCorcPolicy = false;
        bool m_restartThreatDetector = false;
        int m_pendingOnAccessProcessReload = 0;

        inline static const std::string OA_FLAG { "av.onaccess.enabled" };
        inline static const std::string SS_FLAG { "safestore.enabled" };
    };
} // namespace Plugin
