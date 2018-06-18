/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef SULDOWNLOADER_CONNECTIONSETUP_H
#define SULDOWNLOADER_CONNECTIONSETUP_H

#include <string>
#include <vector>
#include "Proxy.h"

namespace SulDownloader
{

    /**
     * Holds enough information to setup a SUL connection to the Warehouse.
     */
    class ConnectionSetup
    {

    public:
        explicit  ConnectionSetup ( const std::string updateLocationURL,Credentials credentials = Credentials(), bool isCacheUpdate = false
                , Proxy proxy = Proxy());

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

#endif //SULDOWNLOADER_CONNECTIONSETUP_H
