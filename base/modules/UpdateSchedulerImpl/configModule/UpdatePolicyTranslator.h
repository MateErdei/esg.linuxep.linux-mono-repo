/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <UpdateScheduler/IMapHostCacheId.h>
#include <Common/Exceptions/IException.h>
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
            std::pair<std::tm, bool> scheduledUpdateTime;
        };

        class PolicyValidationException : public Common::Exceptions::IException
        {
        public:
            static void validateOrThrow( SettingsHolder & settingsHolder);
            using Common::Exceptions::IException::IException;
        };

        class UpdatePolicyTranslator
                : public virtual UpdateScheduler::IMapHostCacheId
        {
        public:
            // may throw PolicyValidationException if the policy does not pass the validation criteria.
            SettingsHolder translatePolicy(const std::string& policyXml);

            std::string cacheID(const std::string& hostname) const override;

            std::string revID() const;

        private:
            struct Cache
            {
                std::string hostname;
                std::string priority;
                std::string id;
            };
            std::vector<Cache> m_Caches;
            std::string m_revID;
        };
    }
}
