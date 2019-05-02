/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Credentials.h"

#include <string>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class Proxy
        {
        public:
            static const std::string NoProxy;

            explicit Proxy(
                std::string url = "",
                suldownloaderdata::ProxyCredentials credentials = suldownloaderdata::ProxyCredentials());

            const suldownloaderdata::ProxyCredentials& getCredentials() const;

            const std::string& getUrl() const;

            bool empty() const;

            bool operator==(const Proxy& rhs) const
            {
                return (m_url == rhs.m_url && m_credentials == rhs.m_credentials);
            }

            bool operator!=(const Proxy& rhs) const { return !operator==(rhs); }

            std::string toStringPostfix() const;

        private:
            std::string m_url;
            suldownloaderdata::ProxyCredentials m_credentials;
        };
    } // namespace suldownloaderdata
} // namespace SulDownloader
