/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <SulDownloader/ConfigurationData.h>

namespace UpdateScheduler
{
    class UpdatePolicyTranslator
    {
    public:
        SulDownloader::ConfigurationData translatePolicy(const std::string &policyXml);
    private:
        struct Cache
        {
            std::string hostname;
            std::string priority;
            std::string id;
        };
        std::vector<Cache> m_Caches;
    };
}



