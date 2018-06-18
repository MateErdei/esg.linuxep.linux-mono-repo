/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
        std::string releaseTag;
        std::string baseVersion;
    };
//TODO improve documentation of ConfigurationData
    /**
     * Holds all the settings that SulDownloader needs to run which includes:
     *  - Information about connection
     *  - Information about the products to download
     *  - Config for log verbosity.
     *
     *  It will be mainly configured from the ConfigurationSettings serialized json.
     *
     */
    class ConfigurationData
    {
    public:
        enum class LogLevel{NORMAL, VERBOSE};
        explicit ConfigurationData(const std::vector<std::string> & sophosLocationURL,  const Credentials& credentials = Credentials(),
                const std::vector<std::string> & updateCache = std::vector<std::string>(), const Proxy& proxy = Proxy());

        const Credentials &getCredentials() const;

        void setCredentials(const Credentials &credentials);

        const std::vector<std::string> &getSophosUpdateUrls() const;

        void setSophosUpdateUrls(const std::vector<std::string> &sophosUpdateUrls);

        const std::vector<std::string> &getLocalUpdateCacheUrls() const;

        void setLocalUpdateCacheUrls(const std::vector<std::string> &localUpdateCacheUrls);

        const Proxy &getProxy() const;

        void setProxy(const Proxy &proxy);

        /**
         * On top of the configured proxy (getProxy) there is the environment proxy that has to be considered.
         * Hence, there will be either:
         *  - 1 proxy to try: configured proxy
         *  - 2 proxies: environment and no proxy
         * @return list of proxies to try to connection.
         */
        std::vector<Proxy> proxiesList() const;

        void setCertificatePath(const std::string &  certificatePath);
        void setUpdateCacheSslCertificatePath(const std::string &  certificatePath);
        void setSystemSslCertificatePath(const std::string &  certificatePath);
        void setInstallationRootPath(const std::string &installationRootPath);
        std::string getInstallationRootPath() const;
        std::string getLocalWarehouseRepository() const;
        std::string getLocalDistributionRepository() const;

        const std::string& getCertificatePath() const ;
        const std::string& getUpdateCacheSslCertificatePath() const;

        void addProductSelection(const ProductGUID &productGUID);
        const std::vector<ProductGUID> & getProductSelection() const;

        const std::string& getSystemSslCertificatePath() const;
        LogLevel getLogLevel() const;

        const std::vector<std::string> & getInstallArguments() const;
        void setInstallArguments(const std::vector<std::string> & installArguments);
//TODO document what are the verifications that are performed.
        bool verifySettingsAreValid();
        bool isVerified() const;

        static ConfigurationData fromJsonSettings( const std::string & settingsString );

    private:
        enum class State{Initialized, Verified, FailedVerified};
        Credentials m_credentials;
        std::vector<std::string> m_sophosUpdateUrls;
        std::vector<std::string> m_localUpdateCacheUrls;
        Proxy m_proxy;
        State m_state;
        std::string m_installationRootPath;
        std::string m_certificatePath;
        std::string m_systemSslCertificatePath;
        std::string m_updateCacheSslCertificatePath;
        std::vector<ProductGUID> m_productSelection;
        std::vector<std::string> m_installArguments;

    };
}

#endif //SULDOWNLOADER_CONFIGURATIONDATA_H
