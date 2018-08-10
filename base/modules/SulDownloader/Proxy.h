/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

#include "Credentials.h"

namespace SulDownloader
{
    class Proxy
    {
    public:
        static const std::string NoProxy;
        explicit Proxy( const std::string & url = "", const Credentials & credentials = Credentials()  );

        const Credentials & getCredentials() const;

        const std::string &getUrl() const;
        bool empty() const;

        bool operator==(const Proxy& rhs) const
        {
            return (m_url==rhs.m_url&&
                    m_credentials==rhs.m_credentials);
        }
        bool operator !=(const Proxy& rhs) const
        {
            return ! operator==(rhs);
        }

    private:

        std::string m_url;
        Credentials m_credentials;
    };
}


