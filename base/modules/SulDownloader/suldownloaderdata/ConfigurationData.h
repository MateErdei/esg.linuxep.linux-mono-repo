// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/ALCPolicy.h"
#include "Common/Policy/ProductSubscription.h"
#include "Common/Policy/Proxy.h"
#include "Common/Policy/UpdateSettings.h"

#include <optional>
#include <string>
#include <vector>

namespace SulDownloader::suldownloaderdata
{
    constexpr char SSPLBaseName[] = "ServerProtectionLinux-Base";

    using ProductSubscription = Common::Policy::ProductSubscription;

    /**
     * Holds all the settings that SulDownloader needs to run which includes:
     *  - Information about connection
     *  - Information about the products to download
     *  - Config for log verbosity.
     *
     *  It will be mainly configured from the ConfigurationSettings serialized json.
     *
     */
    class ConfigurationData : public Common::Policy::UpdateSettings
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
            Common::Policy::Credentials credentials = Common::Policy::Credentials(),
            const std::vector<std::string>& updateCache = std::vector<std::string>(),
            Common::Policy::Proxy policyProxy = Common::Policy::Proxy());

        explicit ConfigurationData(const Common::Policy::UpdateSettings&);

        /**
         * Gets the credentials used to connect to the remote warehouse repository.
         * @return Credential object providing access to stored username and password.
         */
        const Common::Policy::Credentials& getCredentials() const;

        /**
         * Sets the credentials used to connect to the remote warehouse repository.
         * @param credentials object providing access to stored username and password.
         */
        void setCredentials(const Common::Policy::Credentials& credentials);

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
        const UpdateCacheHosts_t& getLocalUpdateCacheUrls() const;

        /**
         * Sets the list of domain urls for the local cache warehouse repositories
         * @param list of local cache (domain url) locations
         */
        void setLocalUpdateCacheUrls(const UpdateCacheHosts_t& localUpdateCacheUrls);

        /**
         * Gets the configured update proxy
         * @return proxy object containing the proxy details.
         */
        const Common::Policy::Proxy& getPolicyProxy() const;

        /**
         * Sets the configured update proxy
         * @param proxy object containing the proxy details.
         */
        void setPolicyProxy(const Common::Policy::Proxy& proxy);

        /**
         * Gets the JWToken
         * @return string containing the latest JWToken
         */
        const std::string& getJWToken() const;

        /**
         * Sets the configured JWToken
         * @param pstring containing the latest JWToken
         */
        void setJWToken(const std::string& token);

        /**
         * Gets the VersigPath
         * @return string containing the latest VersigPath
         */
        const std::string& getVersigPath() const;

        /**
         * Sets the configured VersigPath
         * @param pstring containing the latest VersigPath
         */
        void setVersigPath(const std::string& path);


        /**
         * Gets the updateCache certificates Path
         * @return string containing the latest updateCache certificates Path
         */
        const std::string& getUpdateCacheCertPath() const;

        /**
         * Sets the configured updateCache certificates Path
         * @param pstring containing the latest updateCache certificates Path
         */
        void setUpdateCacheCertPath(const std::string& path);
        /**
         * gets the tenant id
         * @return string containing the tenant id
         */
        const std::string& getTenantId() const;

        /**
         * Sets the configured tenant Id value.
         * @param tenantId
         */
        void setTenantId(const std::string& tenantId);

        /**
         * gets the device Id
         * @return the device Id value
         */
        const std::string& getDeviceId() const;

        /**
         * Sets the configured device Id value
         * @param deviceId
         */
        void setDeviceId(const std::string& deviceId);


        /**
         * On top of the configured proxy (getProxy) there is the environment proxy that has to be considered.
         * Hence, there will be either:
         *  - 1 proxy to try: configured proxy
         *  - 2 proxies: environment and no proxy
         * @return list of proxies to try to connection.
         */
        std::vector<Common::Policy::Proxy> proxiesList() const;


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
         * Set whether to use force an reinstall on non paused customers
         * @param doForcedUpdate
         */
        void setDoForcedUpdate(bool doForcedUpdate);

        /**
         * Get whether to use force an reinstall on non paused customers
         * @return
         */
        [[nodiscard]] bool getDoForcedUpdate() const;

        /**
         * Set whether to use force an reinstall on paused customers
         * @param doForcedPausedUpdate
         */
        void setDoForcedPausedUpdate(bool doForcedPausedUpdate);

        /**
         * Get whether to use force an reinstall on paused customers
         * @return
         */
        [[nodiscard]] bool getDoPausedForcedUpdate() const;

        /**
         * Used to verify all required settings stored in the ConfigurationData object
         * @test sophosUpdateUrls list is not empty
         * @test productSelection list is not empty
         * @test first item in m_productSelection is marked as the primary product.
         * @test installationRootPath is a valid directory
         * @test localWarehouseRepository is a valid directory
         * @test localDistributionRepository is a valid directory
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
        static std::optional<Common::Policy::Proxy> proxyFromSavedProxyUrl(const std::string& savedProxyURL);


        static std::optional<Common::Policy::Proxy> currentMcsProxy();

    private:
        enum class State
        {
            Initialized,
            Verified,
            FailedVerified
        };

        std::vector<std::string> m_sophosUpdateUrls;
        State m_state;
        std::string m_versigPath;
        std::string m_updateCacheCertPath;
        std::string m_jwToken;
        std::string m_tenantId;
        std::string m_deviceId;
        LogLevel m_logLevel;
        bool m_forceReinstallAllProducts;
        bool m_doForcedUpdate = false;
        bool m_doForcedPausedUpdate = false;
        Common::Policy::WeekDayAndTimeForDelay m_scheduledUpdate;
    };
} // namespace SulDownloader::suldownloaderdata
