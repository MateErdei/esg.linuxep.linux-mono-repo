// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/StoppableSleeper.h"
#include "common/ThreatDetector/SusiSettings.h"
#include "scan_messages/ProcessControlSerialiser.h"

#include "Common/XmlUtilities/AttributesMap.h"

#include <nlohmann/json.hpp>

#define USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
#define USE_SXL_ENABLE_FROM_CORC_POLICY

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
         * For example there are 2 SAV polices.
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
        static bool isLookupEnabledFromSavPolicy(const AttributesMap& policy);
        static bool isLookupEnabledFromCorcPolicy(const AttributesMap& policy);
        [[nodiscard]] bool getSXL4LookupsEnabled() const;

        static void setOnAccessConfiguredTelemetry(bool enabled);

        void processFlagSettings(const std::string& flagsJson);
        [[nodiscard]] bool isSafeStoreEnabled() const;
        [[nodiscard]] bool shouldSafeStoreQuarantineMl() const;

        /**
         * Indicates if the previous policy requires ThreatDetector to update SUSI configuration.
         * Only valid between policy changes.
         */
        [[nodiscard]] bool restartThreatDetector() const
        {
            return m_restartThreatDetector;
        }

        /**
         * Indicates if the previous policy requires ThreatDetector to update SUSI configuration.
         * Only valid between policy changes.
         */
        [[nodiscard]] bool reloadThreatDetectorConfiguration() const
        {
            return reloadThreatDetectorConfiguration_;
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
        void processSusiSettingsFromCOREpolicy(const AttributesMap& policy);
        void processOnAccessSettingsFromSAVpolicy(const AttributesMap& policy);

    private:
        void processOnAccessFlagSettings(const nlohmann::json& flagsJson);
        void processSafeStoreFlagSettings(const nlohmann::json& flagsJson);

        void loadSusiSettings();
        void saveSusiSettings(const std::string& policyName);

        static std::vector<std::string> extractListFromXML(
            const AttributesMap& policy,
            const std::string& entityFullPath);

        static nlohmann::json readOnAccessConfig();
        void writeOnAccessConfig(const nlohmann::json& configJson);


        inline static const std::string OA_FLAG{ "av.onaccess.enabled" };
        inline static const std::string SS_FLAG{ "safestore.enabled" };
        inline static const std::string SS_ML_FLAG{ "safestore.quarantine-ml" };

        IStoppableSleeperSharedPtr m_sleeper;
        std::string m_customerId;
        common::ThreatDetector::SusiSettings m_threatDetectorSettings;

        int m_pendingOnAccessProcessReload = 0;

        bool m_safeStoreEnabled = false;
        bool m_safeStoreQuarantineMl = false;

        bool m_gotFirstSavPolicy = false;
        bool m_gotFirstAlcPolicy = false;
        bool m_gotFirstCorcPolicy = false;

        bool m_susiSettingsWritten = false;

        void resetTemporaryMarkerBooleans();

    protected:
        /**
         * Temporary boolean.
         * Indicates if the previous policy requires ThreatDetector to restart.
         * Only valid between policy changes.
         */
        bool m_restartThreatDetector = false;

        /**
         * Temporary boolean.
         * Indicates if the previous policy requires ThreatDetector to update SUSI configuration.
         * Only valid between policy changes.
         */
        bool reloadThreatDetectorConfiguration_ = false;

        static bool isLookupUrlValid(const std::string& url);

    };
} // namespace Plugin
