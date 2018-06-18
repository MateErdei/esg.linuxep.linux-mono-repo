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


        explicit ConfigurationData(const std::vector<std::string> & sophosLocationURL,  Credentials credentials = Credentials(), const std::vector<std::string> & updateCache = std::vector<std::string>(), Proxy proxy = Proxy());

        /**
         * Gets the credentials used to connect to the remote warehouse repository.
         * @return Credential object providing access to stored username and password.
         */
        const Credentials &getCredentials() const;

        /**
         * Sets the credentials used to connect to the remote warehouse repository.
         * @param credentials object providing access to stored username and password.
         */
        void setCredentials(const Credentials &credentials);


        /**
         * Gets the list of domain urls for the sophos warehouse repositories
         * @return list of sophos (domain url) locations
         */
        const std::vector<std::string> &getSophosUpdateUrls() const;

        /**
         * Sets the list of domain urls for the sophos warehouse repositories
         * @param list of sophos (domain url) locations
         */
        void setSophosUpdateUrls(const std::vector<std::string> &sophosUpdateUrls);

        /**
         * Gets the list of domain urls for the local cache warehouse repositories
         * @return list of local cache (domain url) locations
         */
        const std::vector<std::string> &getLocalUpdateCacheUrls() const;

        /**
         * Sets the list of domain urls for the local cache warehouse repositories
         * @param list of local cache (domain url) locations
         */
        void setLocalUpdateCacheUrls(const std::vector<std::string> &localUpdateCacheUrls);

        /**
         * Gets the configured update proxy
         * @return proxy object containing the proxy details.
         */
        const Proxy &getProxy() const;

        /**
         * Sets the configured update proxy
         * @param proxy object containing the proxy details.
         */
        void setProxy(const Proxy &proxy);

        /**
         * On top of the configured proxy (getProxy) there is the environment proxy that has to be considered.
         * Hence, there will be either:
         *  - 1 proxy to try: configured proxy
         *  - 2 proxies: environment and no proxy
         * @return list of proxies to try to connection.
         */
        std::vector<Proxy> proxiesList() const;

        /**
         * Sets the path to the certificates used to validate the warehouse repository files and distributed files.
         * The warehouse repository files will be signed with the same sophos certificate used to sign application files.
         * @param certificatePath, path containing the sophos certificates
         */
        void setCertificatePath(const std::string &  certificatePath);

        /**
         * Set the update cache ssl certificate path used to validate the local update cache https url connection.
         * These ssl certificates will be provided via Sophos central policy, when the update cache is machine is configured.
         * @param certificatePath, path to the local cache ssl certificates store.
         */
        void setUpdateCacheSslCertificatePath(const std::string &  certificatePath);

        /**
         * Set the system ssl certificate path used to validate https url connections.  The system certificate store should
         * contain the required root certificates to validate the sophos website when connecting over https.
         * @param certificatePath, path to the system ssl certificates store.
         */
        void setSystemSslCertificatePath(const std::string &  certificatePath);

        /**
         * Sets the installation root path, this path is used as the relative path of all other application paths.
         * @param installationRootPath, location where the product is installed.
         */
        void setInstallationRootPath(const std::string &installationRootPath);

        /**
         * Gets the installation root path, this path is used as the relative path of all other application paths.
         * @return installationRootPath, location where the product is installed.
         */
        std::string getInstallationRootPath() const;

        /**
         * Gets the path to the local warehouse repository relative to the install root path.
         * @return path to the local warehouse repository.
         */
        std::string getLocalWarehouseRepository() const;

        /**
         * Gets the path to the local distribution repository relative to the install root path.
         * @return path to the local distribution repository.
         */
        std::string getLocalDistributionRepository() const;

        /**
         * Gets the path to the certificates used to validate the warehouse repository files and distributed files.
         * The warehouse repository files will be signed with the same sophos certificate used to sign application files.
         * @return certificatePath, path containing the sophos certificates
         */
        std::string getCertificatePath() const ;

        /**
         * Get the update cache ssl certificate path used to validate the local update cache https url connection.
         * These ssl certificates will be provided via Sophos central policy, when the update cache is machine is configured.
         * @return certificatePath, path to the local cache ssl certificates store.
         */
        std::string getUpdateCacheSslCertificatePath() const;

        /**
         * Set the system ssl certificate path used to validate https url connections.  The system certificate store should
         * contain the required root certificates to validate the sophos website when connecting over https.
         * @return certificatePath, path to the system ssl certificates store.
         */
        std::string getSystemSslCertificatePath() const;

        /**
         * Adds the given productGUID containing the product details to the product selection list
         * @param productGUID, object containing the product details.
         */
        void addProductSelection(const ProductGUID &productGUID);

        /**
         * Gets the list productGUID objetcs containing the details of all the selected products
         * @return list of ProductGUID objects.
         */
        const std::vector<ProductGUID> getProductSelection() const;

        /**
         * Gets the log level parameter that has been set for the application.
         * @return LogLevel, enum specifying the set log level.
         */
        LogLevel getLogLevel() const;


        /**
         * Gets the list of arguments that need to be passed to all product install.sh scripts.
         * @return list of command line arguments.
         */
        std::vector<std::string> getInstallArguments() const;

        /**
         * Sets the list of arguments that need to be passed to all product install.sh scripts.
         * @param installArguments, the list of required command line arguments.
         */
        void setInstallArguments(const std::vector<std::string> & installArguments);

        /**
         * Used to verify all required settings stored in the ConfigurationData object
         * @test sophosUpdateUrls list is not empty
         * @test productSelection list is not empty
         * @test first item in m_productSelection is marked as the primary product.
         * @test installationRootPath is a valid directory
         * @test localWarehouseRepository is a valid directory
         * @test localDistributionRepository is a valid directory
         * @test certificatePath is valid directory, which contains the rootca.crt and "ps_rootca.crt" files
         * @test systemSslCertificatePath is a valid directory
         * @test updateCacheSslCertificatePath is a valid directory
         * @test updateCacheSslCertificatePath is a valid directory
         * @return true, if all required settings are valid, false otherwise
         */
        bool verifySettingsAreValid();

        /**
         * Used to test if the processed configuration data is valid.
         * @return true, if configuration data is valid, false otherwise.
         */
        bool isVerified() const;

        /**
         * Converts a specific json string into a configuration data object.
         * @param settingsString, string contain a json formatted data the will be used to create the ConfigurationData object.
         * @return configurationData object containing the settings from the json data
         * @throws SulDownloaderException if settingsString cannot be converted.
         */
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
