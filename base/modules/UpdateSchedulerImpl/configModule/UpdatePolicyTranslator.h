/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Exceptions/IException.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <UpdateScheduler/IMapHostCacheId.h>

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <chrono>
#include <memory>

namespace UpdateSchedulerImpl
{
    class UpdatePolicyTelemetry
    {
    public:
        void updateSubscriptions(std::vector<std::tuple<std::string, std::string, std::string>> subscriptions);
        void setSDDSid(const std::string & );
        void resetTelemetry(Common::Telemetry::TelemetryHelper& );
        void setSubscriptions(Common::Telemetry::TelemetryHelper& );
    private:
        struct WarehouseTelemetry
        {
            std::vector<std::tuple<std::string, std::string, std::string>> m_subscriptions;
            std::string m_sddsid;

        };
        WarehouseTelemetry warehouseTelemetry;
    };


    namespace configModule
    {
        struct SettingsHolder
        {
            using WeekDayAndTimeForDelay = SulDownloader::suldownloaderdata::WeekDayAndTimeForDelay;
            using SulDownloaderConfigurationData = SulDownloader::suldownloaderdata::ConfigurationData;

            SulDownloaderConfigurationData configurationData;
            std::string updateCacheCertificatesContent;
            std::chrono::minutes schedulerPeriod;
            WeekDayAndTimeForDelay weeklySchedule;
        };

        class PolicyValidationException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;
        };

        class UpdatePolicyTranslator : public virtual UpdateScheduler::IMapHostCacheId
        {
        public:
            // may throw PolicyValidationException if the policy does not pass the validation criteria.
            SettingsHolder translatePolicy(const std::string& policyXml);
            static std::string calculateSulObfuscated(const std::string& user, const std::string& pass);

            std::string cacheID(const std::string& hostname) const override;

            std::string revID() const;
            UpdatePolicyTranslator();
            ~UpdatePolicyTranslator();
        private:
            SettingsHolder _translatePolicy(const std::string& policyXml);
            struct Cache
            {
                std::string hostname;
                std::string priority;
                std::string id;
            };
            std::vector<Cache> sortUpdateCaches(const std::vector<Cache>& caches);
            std::vector<Cache> m_Caches;
            std::string m_revID;
            UpdatePolicyTelemetry m_updatePolicy;
        };
    } // namespace configModule
} // namespace UpdateSchedulerImpl
