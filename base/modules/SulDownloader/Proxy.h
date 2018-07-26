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

        explicit Proxy( const std::string & url = "", const Credentials & credentials = Credentials()  );

        const Credentials & getCredentials() const;

        const std::string &getUrl() const;
        bool empty() const;

    private:

        std::string m_url;
        Credentials m_credentials;
    };
}


