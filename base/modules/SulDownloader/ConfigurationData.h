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


    struct ProductGUID
    {
        std::string Name;
        bool Primary;
        bool Prefix;
    };

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
        void setCertificatePath(const std::string &  certificatePath);
        void setLocalRepository(const std::string & localRepository);
        std::string getCertificatePath() const ;
        std::string getLocalRepository() const;
        void addProductSelection(const ProductGUID &productGUID);
        const std::vector<ProductGUID> getProductSelection() const;


        bool verifySettingsAreValid();
        bool isVerified() const;

    private:
        enum class State{Initialized, Verified, FailedVerified};
        Credentials m_credentials;
        std::vector<std::string> m_sophosUpdateUrls;
        std::vector<std::string> m_localUpdateCacheUrls;
        Proxy m_proxy;
        State m_state;
        std::string m_localRepository;
        std::string m_certificatePath;
        std::vector<ProductGUID> m_productSelection;

    };
}

#endif //SULDOWNLOADER_CONFIGURATIONDATA_H
