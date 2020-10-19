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
        void clearSubscriptions();
        void addSubscription(const std::string & rigidname, const std::string & fixedVersion);
        void setSDDSid(const std::string & );
        void resetTelemetry(Common::Telemetry::TelemetryHelper& );
    private:
        struct WarehouseTelemetry
        {
            std::vector<std::pair<std::string, std::string>> m_subscriptions;
            std::string m_sddsid;

        };
        WarehouseTelemetry warehouseTelemetry;
    };


    namespace configModule
    {
        struct SettingsHolder
        {
            SulDownloader::suldownloaderdata::ConfigurationData configurationData;
            std::string updateCacheCertificatesContent;
            std::chrono::minutes schedulerPeriod;
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
