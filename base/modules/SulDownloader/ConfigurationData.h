//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_CONFIGURATIONDATA_H
#define SULDOWNLOADER_CONFIGURATIONDATA_H

#include <string>
#include <vector>

#include "Credentials.h"
#include "Proxy.h"

namespace SulDownloader
{

    class ConfigurationData
    {
    public:

        explicit ConfigurationData(const std::vector<std::string> & sophosLocationURL,  Credentials credentials = Credentials(), const std::vector<std::string> & updateCache = std::vector<std::string>(), Proxy proxy = Proxy());

        const Credentials &getCredentials() const;

        void setCredentials(const Credentials &credentials);

        const std::vector<std::string> &getSophosUpdateUrls() const;

        void setSophosUpdateUrls(const std::vector<std::string> &sophosUpdateUrls);

        const std::vector<std::string> &getLocalUpdateCacheUrls() const;

        void setLocalUpdateCacheUrls(const std::vector<std::string> &localUpdateCacheUrls);

        const Proxy &getProxy() const;

        void setProxy(const Proxy &proxy);
        std::string getCertificatePath() const ;
        std::string getLocalRepository() const;

    private:
        Credentials m_credentials;
        std::vector<std::string> m_sophosUpdateUrls;
        std::vector<std::string> m_localUpdateCacheUrls;
        Proxy m_proxy;


    };
}

#endif //SULDOWNLOADER_CONFIGURATIONDATA_H
