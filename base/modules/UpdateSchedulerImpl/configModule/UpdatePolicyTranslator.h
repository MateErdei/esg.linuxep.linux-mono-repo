/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <UpdateScheduler/IMapHostCacheId.h>
#include <chrono>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        struct SettingsHolder
        {
            SulDownloader::ConfigurationData configurationData;
            std::string updateCacheCertificatesContent;
            std::chrono::minutes schedulerPeriod;
        };

        class UpdatePolicyTranslator
                : public virtual UpdateScheduler::IMapHostCacheId
        {
        public:
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
