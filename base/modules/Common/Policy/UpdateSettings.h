// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "ProductSubscription.h"
#include "Proxy.h"

#include <string>
#include <utility>
#include <vector>

namespace Common::Policy
{
    class UpdateSettings
    {
    public:
        static const std::vector<std::string> DefaultSophosLocationsURL;
        using UpdateCacheHosts_t = std::vector<std::string>;

        /**
         * Sets the list of hostnames for the local cache warehouse repositories
         * @param list of UpdateCache hostnames
         */
        void setLocalUpdateCacheHosts(const UpdateCacheHosts_t& localUpdateCacheHosts)
        {
            localUpdateCacheHosts_ = localUpdateCacheHosts;
        }

        [[nodiscard]] UpdateCacheHosts_t getLocalUpdateCacheHosts() const
        {
            return localUpdateCacheHosts_;
        }

        /**
         * Sets the configured update proxy
         * @param proxy object containing the proxy details.
         */
        void setPolicyProxy(const Proxy& proxy)
        {
            policyProxy_ = proxy;
        }
        [[nodiscard]] Proxy getPolicyProxy() const
        {
            return policyProxy_;
        }

        /**
         * Set the primary subscription. The primary subscription is meant to be used to enforce that it can never
         * be automatically uninstalled if not present in the warehouse. For sspl it is meant to target base.
         * @param productSubscription
         */
        void setPrimarySubscription(const ProductSubscription& productSubscription)
        {
            primarySubscription_ = productSubscription;
        }

        [[nodiscard]] ProductSubscription getPrimarySubscription() const
        {
            return primarySubscription_;
        }

        /**
         * Set the list of other products to be installed. Plugins
         * @param productsSubscription, subscriptions to plugins such as AV, EDR etc
         */
        void setProductsSubscription(std::vector<ProductSubscription> productsSubscriptions)
        {
            productSubscriptions_ = std::move(productsSubscriptions);
        }

        [[nodiscard]] std::vector<ProductSubscription> getProductsSubscription() const
        {
            return productSubscriptions_;
        }

        /**
         * Set the list of features that the downloaded products should have.
         * Valid values are for example CORE, MDR, SENSORS. It is meant to be used
         * as a second filter level to select only products that have the featues that the
         * EndPoint is meant to have.
         * @param features
         */
        void setFeatures(std::vector<std::string> features)
        {
            features_ = std::move(features);
        }

        [[nodiscard]] std::vector<std::string> getFeatures() const
        {
            return features_;
        }


        /**
         * Sets the list of arguments that need to be passed to all product install.sh scripts.
         * @param installArguments, the list of required command line arguments.
         */
        void setInstallArguments(const std::vector<std::string>& installArguments)
        {
            installArguments_ = installArguments;
        }
        [[nodiscard]] std::vector<std::string> getInstallArguments() const
        {
            return installArguments_;
        }

        /**
         * Set the list of mandatory manifest (relative) file paths that must exist for all packages.
         * @param manifestNames
         */
        void setManifestNames(const std::vector<std::string>& manifestNames)
        {
            manifestNames_ = manifestNames;
        }
        [[nodiscard]] std::vector<std::string> getManifestNames() const
        {
            return manifestNames_;
        }

        /**
         * Set the list of optional manifest (relative) file paths that are may exist for packages.
         * @param optionalManifestNames
         */
        void setOptionalManifestNames(const std::vector<std::string>& optionalManifestNames)
        {
            optionalManifestNames_ = optionalManifestNames;
        }
        [[nodiscard]] std::vector<std::string> getOptionalManifestNames() const
        {
            return optionalManifestNames_;
        }

        void setUseSlowSupplements(bool useSlowSupplements)
        {
            useSlowSupplements_ = useSlowSupplements;
        }
        [[nodiscard]] bool getUseSlowSupplements() const
        {
            return useSlowSupplements_;
        }

        void setCredentials(const Credentials& creds)
        {
            credentials_ = creds;
        }

        [[nodiscard]] Credentials getCredentials() const
        {
            return credentials_;
        }

        using url_list_t = std::vector<std::string>;

        void setSophosLocationURLs(const url_list_t& urls)
        {
            sophosLocationURLs_ = urls;
        }

        [[nodiscard]] url_list_t getSophosLocationURLs() const
        {
            return sophosLocationURLs_;
        }

    protected:
        UpdateCacheHosts_t localUpdateCacheHosts_;
        ProductSubscription primarySubscription_;
        std::vector<ProductSubscription> productSubscriptions_;
        url_list_t sophosLocationURLs_;
        std::vector<std::string> features_;
        std::vector<std::string> installArguments_;
        std::vector<std::string> manifestNames_;
        std::vector<std::string> optionalManifestNames_;
        Proxy policyProxy_;
        Credentials credentials_;
        bool useSlowSupplements_ = false;
    };
}
