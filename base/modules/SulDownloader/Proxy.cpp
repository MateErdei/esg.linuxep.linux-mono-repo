//
// Created by pair on 05/06/18.
//

#include "Proxy.h"

namespace SulDownloader
{

    Proxy::Proxy(const std::string &url, const SulDownloader::Credentials &credentials)
            : m_url( url )
            , m_credentials( credentials)
    {

    }

    const Credentials &Proxy::getCredentials() const
    {
        return m_credentials;
    }


    const std::string &Proxy::getUrl() const
    {
        return m_url;
    }

    bool Proxy::empty() const
    {
        return m_url.empty();
    }


}

