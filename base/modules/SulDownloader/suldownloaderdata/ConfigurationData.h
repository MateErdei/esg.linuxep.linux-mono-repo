/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Credentials.h"
#include "Proxy.h"
#include "WeekDayAndTimeForDelay.h"

#include <string>
#include <vector>
#include <optional>

namespace SulDownloader::suldownloaderdata
{
    constexpr char SSPLBaseName[] = "ServerProtectionLinux-Base";

    class ProductSubscription
    {
        std::string m_rigidName;
        std::string m_baseVersion;
        std::string m_tag;
        std::string m_fixedVersion;

    public:
        ProductSubscription(
            const std::string& rigidName,
            const std::string& baseVersion,
            const std::string& tag,
            const std::string& fixedVersion) :
            m_rigidName(rigidName),
            m_baseVersion(baseVersion),
            m_tag(tag),
            m_fixedVersion(fixedVersion)
        {
        }
        ProductSubscription() {}
        const std::string& rigidName() const { return m_rigidName; }
        const std::string& baseVersion() const { return m_baseVersion; }
        const std::string& tag() const { return m_tag; }
        const std::string& fixedVersion() const { return m_fixedVersion; }
        [[nodiscard]] std::string toString() const;

        bool operator==(const ProductSubscription& rhs) const
        {
            return (
                (m_rigidName == rhs.m_rigidName) && (m_baseVersion == rhs.m_baseVersion) && (m_tag == rhs.m_tag) &&
                (m_fixedVersion == rhs.m_fixedVersion));
        }

        bool operator!=(const ProductSubscription& rhs) const { return !operator==(rhs); }
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
        /**
         * If the path is not set for the Ssl
         */
        static const std::string DoNotSetSslSystemPath;
        static const std::vector<std::string> DefaultSophosLocationsURL;

        enum class LogLevel
        {
            NORMAL,
            VERBOSE
        };
        explicit ConfigurationData(
            const std::vector<std::string>& sophosLocationURL = {},
            Credentials credentials = Credentials(),
            const std::vector<std::string>& updateCache = std::vector<std::string>(),
            Proxy policyProxy = Proxy());

        /**
         * Gets the credentials used to connect to the remote warehouse repository.
         * @return Credential object providing access to stored username and password.
         */
        const Credentials& getCredentials() const;

        /**
         * Sets the credentials used to connect to the remote warehouse repository.
         * @param credentials object providing access to stored username and password.
         */
        void setCredentials(const Credentials& credentials);

        /**
         * Gets the list of domain urls for the sophos warehouse repositories
         * @return list of sophos (domain url) locations
         */
        const std::vector<std::string>& getSophosUpdateUrls() const;

        /**
         * Sets the list of domain urls for the sophos warehouse repositories
         * @param list of sophos (domain url) locations
         */
        void setSophosUpdateUrls(const std::vector<std::string>& sophosUpdateUrls);

        /**
         * Gets the list of domain urls for the local cache warehouse repositories
         * @return list of local cache (domain url) locations
         */
        const std::vector<std::string>& getLocalUpdateCacheUrls() const;

        /**
         * Sets the list of domain urls for the local cache warehouse repositories
         * @param list of local cache (domain url) locations
         */
        void setLocalUpdateCacheUrls(const std::vector<std::string>& localUpdateCacheUrls);

        /**
         * Gets the configured update proxy
         * @return proxy object containing the proxy details.
         */
        const Proxy& getPolicyProxy() const;

        /**
         * Sets the configured update proxy
         * @param proxy object containing the proxy details.
         */
        void setPolicyProxy(const Proxy& proxy);

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
         * The warehouse repository files will be signed with the same sophos certificate used to sign application
         * files.
         * @param certificatePath, path containing the sophos certificates
         */
        void setCertificatePath(const std::string& certificatePath);

        /**
         * Set the update cache ssl certificate path used to validate the local update cache https url connection.
         * These ssl certificates will be provided via Sophos central policy, when the update cache is machine is
         * configured.
         * @param certificatePath, path to the local cache ssl certificates store.
         */
        void setUpdateCacheSslCertificatePath(const std::string& certificatePath);

        /**
         * Set the system ssl certificate path used to validate https url connections.  The system certificate store
         * should contain the required root certificates to validate the sophos website when connecting over https.
         * @param certificatePath, path to the system ssl certificates store.
         */
        void setSystemSslCertificatePath(const std::string& certificatePath);

        /**
         * Sets the installation root path, this path is used as the relative path of all other application paths.
         * @param installationRootPath, location where the product is installed.
         */
        void setInstallationRootPath(const std::string& installationRootPath);

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
         * The warehouse repository files will be signed with the same sophos certificate used to sign application
         * files. Usually it returns: ApplicationPathManager::getUpdateCertificatesPath()
         * @return certificatePath, path containing the sophos certificates
         */
        std::string getCertificatePath() const;

        /**
         * Get the update cache ssl certificate path used to validate the local update cache https url connection.
         * These ssl certificates will be provided via Sophos central policy, when the update cache is machine is
         * configured.
         * @return certificatePath, path to the local cache ssl certificates store.
         */
        const std::string& getUpdateCacheSslCertificatePath() const;

        /**
         * Set the system ssl certificate path used to validate https url connections.  The system certificate store
         * should contain the required root certificates to validate the sophos website when connecting over https.
         * @return certificatePath, path to the system ssl certificates store.
         */
        const std::string& getSystemSslCertificatePath() const;

        /**
         * Set the primary subscription. The primary subscription is meant to be used to enforce that it can never
         * be automatically uninstalled if not present in the warehouse. For sspl it is meant to target base.
         * @param productSubscription
         */
        void setPrimarySubscription( const ProductSubscription& productSubscription);
        /** Set the list of other products to be installed. In sspl it is meant to target Sensors, plugins, etc
         */
        void setProductsSubscription(const std::vector<ProductSubscription>& productsSubscriptions);

        /**
         * Access to the primary subscription
         * @return
         */

        const ProductSubscription& getPrimarySubscription() const;
        /** Access to the list of subscriptions
         * @return
         */
        const std::vector<ProductSubscription>& getProductsSubscription() const;

        /**
         * Gets the log level parameter that has been set for the application.
         * @return LogLevel, enum specifying the set log level.
         */
        LogLevel getLogLevel() const;

        /**
         * Set the default log level.
         * @param level
         */
        void setLogLevel(LogLevel level);

        /**
         * Set flag to force running the install scripts for all products
         * @param forceReinstallAllProducts
         */
        void setForceReinstallAllProducts(bool forceReinstallAllProducts);

        /**
         * Get flag used to indicate install.sh scripts for all products should be invoked during update.
         * @return true if set, false otherwise.
         */
        bool getForceReinstallAllProducts() const;

        /**
         * Gets the list of arguments that need to be passed to all product install.sh scripts.
         * @return list of command line arguments.
         */
        const std::vector<std::string>& getInstallArguments() const;

        /**
         * Sets the list of arguments that need to be passed to all product install.sh scripts.
         * @param installArguments, the list of required command line arguments.
         */
        void setInstallArguments(const std::vector<std::string>& installArguments);

        /**
         * Get the list of mandatory manifest (relative) file paths that must exist for all the products downloaded.
         * @return list of manifest names.
         */
        const std::vector<std::string>& getManifestNames() const;

        /**
         * Set the list of mandatory manifest (relative) file paths that must exist for all packages.
         * @param manifestNames
         */
        void setManifestNames(const std::vector<std::string>& optionalManifestNames);

        /**
         * Get the list of possible optional manifest f(relative) file paths for the products downloaded.
         * @return list of optional manifest names.
         */
        const std::vector<std::string>& getOptionalManifestNames() const;

        /**
         * Set the list of optional manifest (relative) file paths that are may exist for packages.
         * @param optionalManifestNames
         */
        void setOptionalManifestNames(const std::vector<std::string>& optionalManifestNames);

        /**
         * Set the list of features that the downloaded products should have.
         * Valid values are for example CORE, MDR, SENSORS. It is meant to be used
         * as a second filter level to select only products that have the featues that the
         * EndPoint is meant to have.
         * @param features
         */
        void setFeatures(const std::vector<std::string>& features);
        /**
         * Access to the features configured.
         * @return
         */
        const std::vector<std::string>& getFeatures() const;

        /**
         * Set whether to download the slow version of some supplements or the normal (fast version).
         *
         * For Local Reputation data, there is the normal supplement, or a slower supplement only updated weekly.
         *
         * See https://jira.sophos.net/browse/LINUXDAR-2076
         * See https://wiki.sophos.net/display/global/Requirements+for+publishing+to+the+slow+release+channel+for+DataSetA+and+LocalRep
         *
         * @param useSlowSupplements
         */
        void setUseSlowSupplements(bool useSlowSupplements);

        /**
         * Get whether to download slow supplements or not.
         * @return
         */
        [[nodiscard]] bool getUseSlowSupplements() const;


        /**
         * Set the schedule to determine whether to download supplements only, or product updates as well as supplements.
         *
         * https://jira.sophos.net/browse/LINUXDAR-2093
         * https://wiki.sophos.net/display/SL/Outside+in+-+Scheduled+and+Suspended
         *
         * @param schedule
         */
        void setSchedule(WeekDayAndTimeForDelay schedule);

        /**
         * Get the schedule to determine whether to download supplements only, or product updates as well as supplements.
         * @return
         */
        [[nodiscard]] WeekDayAndTimeForDelay getSchedule() const;

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
        [[nodiscard]] bool isVerified() const;

        /**
         * Converts a specific json string into a configuration data object.
         * @param settingsString, string contain a json formatted data the will be used to create the
         * ConfigurationData object.
         * @return configurationData object containing the settings from the json data
         * @throws SulDownloaderException if settingsString cannot be converted.
         */
        static ConfigurationData fromJsonSettings(const std::string& settingsString);

        /**
         * Serialize the contents of an instance of ConfigurationData into a json representation.
         * Calling ::fromJsonSettings to the output of toJsonSettings yield equivalent ConfigurationData.
         *
         * @paramn configurationData object containing the settings
         * @param settingsString, string contain a json formatted data representing the state of configurationData
         */
        static std::string toJsonSettings(const ConfigurationData&);

        /**
         * Retrieve the Proxy URL and Credentials for authenticated proxy in the format http(s)://username:password@192.168.0.1:8080
         * proxyurl without credentials is passed through as is.
         * @param savedProxyURL the string with proxy url and optionally credentials as can be used on command line
         * @return Proxy object
         */
        static std::optional<Proxy> proxyFromSavedProxyUrl(const std::string& savedProxyURL);

    private:
        enum class State
        {
            Initialized,
            Verified,
            FailedVerified
        };

        Credentials m_credentials;
        std::vector<std::string> m_sophosUpdateUrls;
        std::vector<std::string> m_localUpdateCacheUrls;
        Proxy m_policyProxy;
        State m_state;
        std::string m_certificatePath;
        std::string m_systemSslCertificatePath;
        std::string m_updateCacheSslCertificatePath;
        std::vector<ProductSubscription> m_productsSubscription;
        ProductSubscription m_primarySubscription;
        std::vector<std::string> m_features;
        std::vector<std::string> m_installArguments;
        LogLevel m_logLevel;
        bool m_forceReinstallAllProducts;
        std::vector<std::string> m_manifestNames;
        std::vector<std::string> m_optionalManifestNames;
        bool m_useSlowSupplements = false;
        WeekDayAndTimeForDelay m_scheduledUpdate;
    };
} // namespace SulDownloader::suldownloaderdata
