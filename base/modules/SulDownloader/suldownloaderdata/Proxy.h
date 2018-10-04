/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

#include "Credentials.h"

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class Proxy
        {
        public:
            static const std::string NoProxy;

            explicit Proxy(const std::string& url = "",
                           const suldownloaderdata::ProxyCredentials& credentials = suldownloaderdata::ProxyCredentials());

            const suldownloaderdata::ProxyCredentials& getCredentials() const;

            const std::string& getUrl() const;

            bool empty() const;

            bool operator==(const Proxy& rhs) const
            {
                return (m_url == rhs.m_url &&
                        m_credentials == rhs.m_credentials);
            }

            bool operator!=(const Proxy& rhs) const
            {
                return !operator==(rhs);
            }

        private:

            std::string m_url;
            suldownloaderdata::ProxyCredentials m_credentials;
        };
    }
}


