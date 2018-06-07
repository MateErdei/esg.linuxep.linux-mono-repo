//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_CONNECTIONSETUP_H
#define SULDOWNLOADER_CONNECTIONSETUP_H

#include <string>
#include <vector>
#include "Proxy.h"

namespace SulDownloader
{

    class ConnectionSetup
    {

    public:
        explicit ConnectionSetup(const std::vector<std::string> & m_sophosLocationURL,  Credentials credentials = Credentials(), const std::string & m_updateCache = "", Proxy m_proxy = Proxy());

        const Credentials &getCredentials() const;

        void setCredentials(const Credentials &credentials);

        const Proxy &getProxy() const;

        void setProxy(const Proxy &proxy);

        const std::string &getUpdateCache() const;

        void setUpdateCache(const std::string &updateCache);

        const std::vector<std::string> & getSophosLocationURL() const;

        void setSophosLocationURL(const std::vector<std::string> &sophosLocationURL);

    private:
        std::vector<std::string> m_sophosLocationURL;
        Credentials m_credentials;
        std::string m_updateCache;
        Proxy m_proxy;


    };
}

#endif //SULDOWNLOADER_CONNECTIONSETUP_H
