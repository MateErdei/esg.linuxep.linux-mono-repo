/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <SulDownloader/ConfigurationData.h>
#include "IMapHostCacheId.h"
#include "IRevId.h"

namespace UpdateScheduler
{

    struct SettingsHolder
    {
        SulDownloader::ConfigurationData configurationData;
        std::string updateCacheCertificatesContent;
    };

    class UpdatePolicyTranslator : public virtual  IMapHostCacheId, public virtual IRevId
    {
    public:
        SettingsHolder translatePolicy(const std::string &policyXml);
        std::string cacheID(const std::string & hostname) const override ;
        std::string revID() const override ;
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



