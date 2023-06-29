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
        // Static utility functions

        /**
         * Converts a specific json string into a configuration data object.
         * @param settingsString, string contain a json formatted data the will be used to create the
         * ConfigurationData object.
         * @return configurationData object containing the settings from the json data
         * @throws SulDownloaderException if settingsString cannot be converted.
         */
        static UpdateSettings fromJsonSettings(const std::string& settingsString);

        /**
         * Serialize the contents of an instance of ConfigurationData into a json representation.
         * Calling ::fromJsonSettings to the output of toJsonSettings yield equivalent ConfigurationData.
         *
         * @paramn configurationData object containing the settings
         * @param settingsString, string contain a json formatted data representing the state of configurationData
         */
        static std::string toJsonSettings(const UpdateSettings&);

        /**
         * Retrieve the Proxy URL and Credentials for authenticated proxy in the format http(s)://username:password@192.168.0.1:8080
         * proxyurl without credentials is passed through as is.
         * @param savedProxyURL the string with proxy url and optionally credentials as can be used on command line
         * @return Proxy object
         */
        static std::optional<Common::Policy::Proxy> proxyFromSavedProxyUrl(const std::string& savedProxyURL);


        static std::optional<Common::Policy::Proxy> currentMcsProxy();

        /**
         * On top of the configured proxy (getProxy) there is the environment proxy that has to be considered.
         * Hence, there will be either:
         *  - 1 proxy to try: configured proxy
         *  - 2 proxies: environment and no proxy
         * @return list of proxies to try to connection.
         */
        static std::vector<Common::Policy::Proxy> proxiesList(const Common::Policy::UpdateSettings&);

        /**
         * If the path is not set for the Ssl
         */
        static const std::string DoNotSetSslSystemPath;
        static const std::vector<std::string> DefaultSophosLocationsURL;


        // Legacy ConfigurationData class

        explicit ConfigurationData(
            const std::vector<std::string>& sophosLocationURL = {},
            Common::Policy::Credentials credentials = Common::Policy::Credentials(),
            const std::vector<std::string>& updateCache = std::vector<std::string>(),
            Common::Policy::Proxy policyProxy = Common::Policy::Proxy());

    private:

        /**
         * Gets the credentials used to connect to the remote warehouse repository.
         * @return Credential object providing access to stored username and password.
         */
        const Common::Policy::Credentials& getCredentials() const;

        /**
         * Sets the list of domain urls for the sophos warehouse repositories
         * @param list of sophos (domain url) locations
         */
        void setSophosUpdateUrls(const std::vector<std::string>& sophosUpdateUrls);

        /**
         * Sets the configured update proxy
         * @param proxy object containing the proxy details.
         */
        void setPolicyProxy(const Common::Policy::Proxy& proxy);

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


    };
} // namespace SulDownloader::suldownloaderdata
