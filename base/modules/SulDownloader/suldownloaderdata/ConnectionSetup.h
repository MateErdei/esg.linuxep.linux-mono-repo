/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>
#include "Proxy.h"

namespace SulDownloader
{

    /**
     * Holds enough information to setup a SUL connection required by the WarehouseRepository.
     */
    class ConnectionSetup
    {

    public:
        explicit  ConnectionSetup ( const std::string & updateLocationURL, const Credentials & credentials = Credentials(), bool isCacheUpdate = false
                , const Proxy&  proxy = Proxy());

        const Credentials &getCredentials() const;

        void setCredentials(const Credentials &credentials);

        const Proxy &getProxy() const;

        void setProxy(const Proxy &proxy);

        const std::string & getUpdateLocationURL() const;

        void setUpdateLocationURL(const std::string & updateLocationURL);

        std::string toString() const;

        bool isCacheUpdate() const;

    private:
        std::string m_updateLocationURL;
        Credentials m_credentials;
        bool m_isUpdateCache;
        Proxy m_proxy;


    };


}


