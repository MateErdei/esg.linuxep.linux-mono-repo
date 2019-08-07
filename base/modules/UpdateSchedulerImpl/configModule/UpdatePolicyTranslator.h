/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Exceptions/IException.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <UpdateScheduler/IMapHostCacheId.h>
#include <UpdateScheduler/ScheduledUpdate.h>

#include <chrono>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        struct SettingsHolder
        {
            SulDownloader::suldownloaderdata::ConfigurationData configurationData;
            std::string updateCacheCertificatesContent;
            std::chrono::minutes schedulerPeriod;
            UpdateScheduler::ScheduledUpdate scheduledUpdate;
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
        };
    } // namespace configModule
} // namespace UpdateSchedulerImpl
