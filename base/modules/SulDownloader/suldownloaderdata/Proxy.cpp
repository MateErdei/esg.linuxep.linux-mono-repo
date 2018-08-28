/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Proxy.h"

namespace SulDownloader
{

    const std::string Proxy::NoProxy("noproxy:");

    Proxy::Proxy(const std::string& url, const SulDownloader::suldownloaderdata::Credentials& credentials)
            : m_url(url)
              , m_credentials(credentials)
    {

    }

    const suldownloaderdata::Credentials& Proxy::getCredentials() const
    {
        return m_credentials;
    }


    const std::string& Proxy::getUrl() const
    {
        return m_url;
    }

    bool Proxy::empty() const
    {
        return m_url.empty() || m_url == NoProxy;
    }


}

