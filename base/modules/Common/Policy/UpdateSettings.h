// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "ESMVersion.h"
#include "ProductSubscription.h"
#include "Proxy.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

#include <string>
#include <utility>
#include <vector>

#if defined(__aarch64__)
#define MACHINEARCHITECTURE "LINUX_ARM"
#elif defined(__x86_64__)
#define MACHINEARCHITECTURE "LINUX_INTEL_LIBC6"
#else
#define MACHINEARCHITECTURE ""
#endif

namespace Common::Policy
{
    const std::string machineArchitecture_ {MACHINEARCHITECTURE};

    class UpdateSettings
    {
    public:
        static const std::vector<std::string> DefaultSophosCDNUrls;
        static const std::string DefaultSophosSusUrl;
        using UpdateCacheHosts_t = std::vector<std::string>;
        using MessageRelayHosts_t = std::vector<std::string>;

        enum class LogLevel
        {
            NORMAL,
            VERBOSE
        };

        /**
         * Sets the list of hostnames for the local cache warehouse repositories
         * @param list of UpdateCache hostnames
         */
        void setLocalUpdateCacheHosts(const UpdateCacheHosts_t& localUpdateCacheHosts)
        {
            localUpdateCacheHosts_ = localUpdateCacheHosts;
        }

        [[nodiscard]] UpdateCacheHosts_t getLocalUpdateCacheHosts() const { return localUpdateCacheHosts_; }

        /**
         * Sets the list of hostnames for the local cache warehouse repositories
         * @param list of MessageRelay hostnames
         */
        void setLocalMessageRelayHosts(const MessageRelayHosts_t& localMessageRelayHosts)
        {
            localMessageRelayHosts_ = localMessageRelayHosts;

        }

        [[nodiscard]] MessageRelayHosts_t getLocalMessageRelayHosts() const {return localMessageRelayHosts_;}

        /**
         * Sets the configured update proxy
         * @param proxy object containing the proxy details.
         */
        void setPolicyProxy(const Proxy& proxy) { policyProxy_ = proxy; }
        [[nodiscard]] Proxy getPolicyProxy() const { return policyProxy_; }

        /**
         * Set the primary subscription. The primary subscription is meant to be used to enforce that it can never
         * be automatically uninstalled if not present in the warehouse. For sspl it is meant to target base.
         * @param productSubscription
         */
        void setPrimarySubscription(const ProductSubscription& productSubscription)
        {
            primarySubscription_ = productSubscription;
        }

        [[nodiscard]] ProductSubscription getPrimarySubscription() const { return primarySubscription_; }

        /**
         * Set the list of other products to be installed. Plugins
         * @param productsSubscription, subscriptions to plugins such as AV, EDR etc
         */
        void setProductsSubscription(std::vector<ProductSubscription> productsSubscriptions)
        {
            productSubscriptions_ = std::move(productsSubscriptions);
        }

        [[nodiscard]] std::vector<ProductSubscription> getProductsSubscription() const { return productSubscriptions_; }

        /**
         * Set the list of features that the downloaded products should have.
         * Valid values are for example CORE, MDR, SENSORS. It is meant to be used
         * as a second filter level to select only products that have the featues that the
         * EndPoint is meant to have.
         * @param features
         */
        void setFeatures(std::vector<std::string> features) { features_ = std::move(features); }

        [[nodiscard]] std::vector<std::string> getFeatures() const { return features_; }

        /**
         * Sets the list of arguments that need to be passed to all product install.sh scripts.
         * @param installArguments, the list of required command line arguments.
         */
        void setInstallArguments(const std::vector<std::string>& installArguments)
        {
            installArguments_ = installArguments;
        }
        [[nodiscard]] std::vector<std::string> getInstallArguments() const { return installArguments_; }

        /**
         * Set the list of mandatory manifest (relative) file paths that must exist for all packages.
         * @param manifestNames
         */
        void setManifestNames(const std::vector<std::string>& manifestNames) { manifestNames_ = manifestNames; }
        [[nodiscard]] std::vector<std::string> getManifestNames() const { return manifestNames_; }

        /**
         * Set the list of optional manifest (relative) file paths that are may exist for packages.
         * @param optionalManifestNames
         */
        void setOptionalManifestNames(const std::vector<std::string>& optionalManifestNames)
        {
            optionalManifestNames_ = optionalManifestNames;
        }
        [[nodiscard]] std::vector<std::string> getOptionalManifestNames() const { return optionalManifestNames_; }

        void setUseSlowSupplements(bool useSlowSupplements) { useSlowSupplements_ = useSlowSupplements; }
        [[nodiscard]] bool getUseSlowSupplements() const { return useSlowSupplements_; }

        /**
         * Handling for the SDDS3 URLs:
         */

        using url_list_t = std::vector<std::string>;

        void setSophosCDNURLs(const url_list_t& urls) { sophosCDNURLs_ = urls; }

        [[nodiscard]] url_list_t getSophosCDNURLs() const { return sophosCDNURLs_; }

        void setSophosSusURL(const std::string& url) { sophosSUSUrl_ = url; }

        [[nodiscard]] std::string getSophosSusURL() const { return sophosSUSUrl_; }
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

        void setForceReinstallAllProducts(const bool forceReinstallAllProducts)
        {
            forceReinstallAllProducts_ = forceReinstallAllProducts;
        }

        /**
         * Get flag used to indicate install.sh scripts for all products should be invoked during update.
         * @return true if set, false otherwise.
         */
        [[nodiscard]] bool getForceReinstallAllProducts() const { return forceReinstallAllProducts_; }

        /**
         * @return string containing the tenant id
         */
        [[nodiscard]] const std::string& getTenantId() const { return tenantId_; }

        /**
         * Sets the configured tenant Id value.
         * @param tenantId
         */
        void setTenantId(const std::string& tenantId) { tenantId_ = tenantId; }

        /**
         * @return the device Id value
         */
        [[nodiscard]] const std::string& getDeviceId() const { return deviceId_; }

        /**
         * Sets the configured device Id value
         * @param deviceId
         */
        void setDeviceId(const std::string& deviceId) { deviceId_ = deviceId; }

        /**
         * @return string containing the latest JWToken
         */
        [[nodiscard]] const std::string& getJWToken() const { return jwToken_; }

        /**
         * Sets the configured JWToken
         * @param pstring containing the latest JWToken
         */
        void setJWToken(const std::string& token) { jwToken_ = token; }

        /**
         * Set whether to use force an reinstall on non paused customers
         * @param doForcedUpdate
         */
        void setDoForcedUpdate(const bool doForcedUpdate) { doForcedUpdate_ = doForcedUpdate; }

        /**
         * @return whether to use force an reinstall on non paused customers
         */
        [[nodiscard]] bool getDoForcedUpdate() const { return doForcedUpdate_; }

        /**
         * Set whether to use v2 sdds3 deltas
         * @param doForcedUpdate
         */
        void setUseSdds3DeltaV2(const bool useSdds3DeltaV2) { useSdds3DeltaV2_ = useSdds3DeltaV2; }

        /**
         * @return whether to use v2 sdds3 deltas
         */
        [[nodiscard]] bool getUseSdds3DeltaV2() const { return useSdds3DeltaV2_; }

        /**
         * Set whether to use force an reinstall on paused customers
         * @param doForcedPausedUpdate
         */
        void setDoForcedPausedUpdate(const bool doForcedPausedUpdate) { doForcedPausedUpdate_ = doForcedPausedUpdate; }

        /**
         * @return whether to use force an reinstall on paused customers
         */
        [[nodiscard]] bool getDoPausedForcedUpdate() const { return doForcedPausedUpdate_; }

        /**
         * Gets the VersigPath
         * @return string containing the latest VersigPath
         */
        const std::string& getVersigPath() const { return versigPath_; }

        /**
         * Sets the configured VersigPath
         * @param pstring containing the latest VersigPath
         */
        void setVersigPath(const std::string& path) { versigPath_ = path; }

        /**
         * Gets the log level parameter that has been set for the application.
         * @return LogLevel, enum specifying the set log level.
         */
        LogLevel getLogLevel() const { return logLevel_; }

        /**
         * Set the default log level.
         * @param level
         */
        void setLogLevel(LogLevel level) { logLevel_ = level; }

        /**
         * Gets the updateCache certificates Path
         * @return string containing the latest updateCache certificates Path
         */
        [[nodiscard]] const std::string& getUpdateCacheCertPath() const { return updateCacheCertPath_; }

        /**
         * Sets the configured updateCache certificates Path
         * @param pstring containing the latest updateCache certificates Path
         */
        void setUpdateCacheCertPath(const std::string& path) { updateCacheCertPath_ = path; }

        /**
         * Gets the path to the local warehouse repository relative to the install root path.
         * @return path to the local warehouse repository.
         */
        std::string getLocalWarehouseRepository() const
        {
            return Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseRepository();
        }

        /**
         * Gets the path to the local distribution repository relative to the install root path.
         * @return path to the local distribution repository.
         */
        std::string getLocalDistributionRepository() const
        {
            return Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository();
        }

        /**
         * @param contains string representing esmVersionToken
         */
        void setEsmVersion(const ESMVersion& esmVersion) { esmVersion_ = esmVersion; }

        /**
         * @return string containing the esmVersionToken
         */
        [[nodiscard]] const ESMVersion& getEsmVersion() const { return esmVersion_; }

    protected:
        ESMVersion esmVersion_;
        UpdateCacheHosts_t localUpdateCacheHosts_;
        MessageRelayHosts_t localMessageRelayHosts_;
        ProductSubscription primarySubscription_;
        std::vector<ProductSubscription> productSubscriptions_;
        url_list_t sophosCDNURLs_;
        std::string sophosSUSUrl_;
        std::vector<std::string> features_;
        std::vector<std::string> installArguments_;
        std::vector<std::string> manifestNames_;
        std::vector<std::string> optionalManifestNames_;
        Proxy policyProxy_;
        std::string jwToken_;
        std::string tenantId_;
        std::string deviceId_;
        std::string versigPath_;
        std::string updateCacheCertPath_;
        bool useSlowSupplements_ = false;
        bool forceReinstallAllProducts_ = false;
        bool doForcedUpdate_ = false;
        bool useSdds3DeltaV2_ = false;
        bool doForcedPausedUpdate_ = false;
        LogLevel logLevel_ = LogLevel::NORMAL;

        enum class State
        {
            Initialized,
            Verified,
            FailedVerified
        };

        State state_ = State::Initialized;

    private:
        bool isProductSubscriptionValid(const ProductSubscription& productSubscription);
    };
} // namespace Common::Policy
