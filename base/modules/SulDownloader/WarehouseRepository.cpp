/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "WarehouseRepository.h"

#include "SULRaii.h"
#include "SULUtils.h"

#include "SulDownloader/suldownloaderdata/UpdateSupplementDecider.h"
#include "suldownloaderdata/Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>

#include <cassert>
#include <sstream>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

using namespace SulDownloader::suldownloaderdata;

namespace
{
    using TagVector = std::vector<Tag>;

    TagVector getTags(SU_PHandle& product)
    {
        TagVector tags;
        int index = 0;

        while (true)
        {
            std::string tag = SulDownloader::SulQueryProductMetadata(product, "R_ReleaseTagsTag", index);
            if (tag.empty())
            {
                break;
            }

            std::string baseversion =
                SulDownloader::SulQueryProductMetadata(product, "R_ReleaseTagsBaseVersion", index);
            std::string label = SulDownloader::SulQueryProductMetadata(product, "R_ReleaseTagsLabel", index);

            tags.emplace_back(tag, baseversion, label);
            index++;
        }
        return tags;
    }

    SubProducts getSubComponents(const std::string& componentName, SU_PHandle& product)
    {
        std::vector<std::string> sulComponents;
        int index = 0;

        while (true)
        {
            std::string sulComponent = SulDownloader::SulQueryProductMetadata(product, "R_SubComponent", index);
            if (sulComponent.empty())
            {
                break;
            }
            sulComponents.push_back(sulComponent);
            index++;
        }
        return ProductMetadata::extractSubProductsFromSulSubComponents(componentName, sulComponents);
    }

    std::vector<std::string> getFeatures(SU_PHandle& product)
    {
        std::vector<std::string> features;
        int index = 0;
        while (true)
        {
            std::string feature = SulDownloader::SulQueryProductMetadata(product, "R_Features", index);
            if (feature.empty())
            {
                break;
            }
            features.emplace_back(feature);
            index++;
        }
        return features;
    }

    std::string joinEntries(const std::vector<std::string>& entries)
    {
        std::stringstream ss;
        bool isfirst = true;
        for (const auto& entry : entries)
        {
            if (isfirst)
            {
                ss << entry;
            }
            else
            {
                ss << ", " << entry;
            }
            isfirst = false;
        }
        return ss.str();
    }

    bool hasError(
        const std::vector<std::pair<SU_PHandle, SulDownloader::suldownloaderdata::DownloadedProduct>>& products)
    {
        for (const auto& product : products)
        {
            if (product.second.hasError())
            {
                return true;
            }
        }
        return false;
    }

    void displayProductTags(SU_PHandle product, const std::vector<std::string>& attributes)
    {
        LOGDEBUG("\nNew Product");
        for (const auto& attribute : attributes)
        {
            std::stringstream all_attributes;
            for (int i = 0; i < 10; i++)
            {
                std::string attribute_value = SulDownloader::SulQueryProductMetadata(product, attribute, i);
                if (attribute_value.empty())
                {
                    break;
                }
                all_attributes << " " << attribute_value;
            }
            LOGDEBUG("Tag: " << attribute << " value:" << all_attributes.str());
        }

        LOGDEBUG("Tag: features value: " << joinEntries(getFeatures(product)));

        std::vector<Tag> tags(getTags(product));
        for (auto& tag : tags)
        {
            LOGDEBUG("Product tag: label=" << tag.label << " baseversion=" << tag.baseversion << " tag=" << tag.tag);
        }
        LOGDEBUG("\n");
    }

    void displayProductTags(SU_PHandle product)
    {
        displayProductTags(
            product,
            { "Line",
              "R_Line",
              "VersionId",
              "Name",
              "PublicationTime",
              "DefaultHomeFolder",
              "Roles",
              "TargetTypes",
              "ReleaseTagsBaseVersion",
              "ReleaseTags",
              "Platforms",
              "Lifestage",
              "SAVLine",
              "ResubscriptionsLine",
              "Resubscriptions",
              "ResubscriptionsVersion",
              "SubComponent" });
    }

} // namespace

namespace SulDownloader
{
    bool WarehouseRepository::tryConnect(
        const ConnectionSetup& connectionSetup,
        bool supplementOnly,
        const ConfigurationData& configurationData)
    {
        assert(m_state == State::Initialized);

        if (!m_session)
        {
            createSession();
        }
        if (hasError())
        {
            m_session.reset();
            return false;
        }
        LOGINFO("Try connection: " << connectionSetup.toString());
        setConnectionSetup(connectionSetup, configurationData);

        if (hasError())
        {
            SULUtils::displayLogs(session(), m_sulLogs);
            m_session.reset();
            return false;
        }

        SU_Result ret = SU_Result_invalid;

        if (supplementOnly)
        {
            ret = SU_readLocalMetadata(session());
        }
        else
        {
            ret = SU_readRemoteMetadata(session());
        }

        if (!SULUtils::isSuccess(ret))
        {
            LOGERROR("Failed to connect to the warehouse: "<<ret);
            SULUtils::displayLogs(session(), m_sulLogs);
            LOGINFO("Failed to connect to: " << m_connectionSetup->toString());
            setError("Failed to connect to warehouse");
            m_error.status = WarehouseStatus::CONNECTIONERROR;
            m_session.reset();
            return false;
        }

        if (!SulDownloader::SulSetLanguage(session(), "en"))
        {
            SULUtils::displayLogs(session(), m_sulLogs);
            LOGWARN("Failed to set language for warehouse session: " << m_connectionSetup->toString());
        }

        // for verbose it will list the entries in the warehouse
        SULUtils::displayLogs(session(), m_sulLogs);
        LOGINFO("Successfully connected to: " << m_connectionSetup->toString());
        m_state = State::Connected;

        // store values from configuration data for later use.
        setRootDistributionPath(configurationData.getLocalDistributionRepository());

        m_supplementOnly = supplementOnly;

        return true;
    }

    void WarehouseRepository::dumpLogs(const WarehouseRepository::SulLogsVector& sulLogs)
    {
        LOGINFO("Sul Library log output:");
        for(const auto& sulLog : sulLogs)
        {
            LOGINFO(sulLog);
        }
        LOGINFO("End of Sul Library log output.");
    }

    void WarehouseRepository::dumpLogs() const
    {
        dumpLogs(m_sulLogs);
    }

    bool WarehouseRepository::hasError() const { return !m_error.Description.empty() || ::hasError(m_products); }


    bool WarehouseRepository::hasImmediateFailError() const
    {
        if (!hasError())
        {
            return false;
        }
        return m_error.status == PACKAGESOURCEMISSING;
    }

    WarehouseError WarehouseRepository::getError() const { return m_error; }

    void WarehouseRepository::synchronize(ProductSelection& selection)
    {
        assert(m_state == State::Connected);
        m_state = State::Synchronized;

        std::vector<std::pair<SU_PHandle, ProductMetadata>> productInformationList;

        // create the ProductMetadata for all the entries in the warehouse
        while (true)
        {
            SU_PHandle product = SU_getProductRelease(session());

            if (product == nullptr)
            {
                break;
            }
            displayProductTags(product);
            ProductMetadata productInformation;

            std::string line = SulQueryProductMetadata(product, "R_Line", 0);
            std::string name = SulQueryProductMetadata(product, "Name", 0);

            if (name.empty())
            {
                name = SulQueryProductMetadata(product, "R_Name", 0);
            }

            std::string productVersion = SulQueryProductMetadata(product, "VersionId", 0);
            std::string defaultHomePath = SulQueryProductMetadata(product, "DefaultHomeFolder", 0);
            std::string baseVersion = SulQueryProductMetadata(product, "ReleaseTagsBaseVersion", 0);
            std::vector<Tag> tags(getTags(product));

            productInformation.setLine(line);
            productInformation.setName(name);
            productInformation.setTags(tags);
            productInformation.setVersion(productVersion);
            productInformation.setFeatures(getFeatures(product));
            productInformation.setBaseVersion(baseVersion);
            productInformation.setDefaultHomePath(defaultHomePath);
            productInformation.setSubProduts(getSubComponents(line, product));
            m_catalogueInfo.addInfo(line, productVersion, name);
            productInformationList.emplace_back(product, productInformation);
        }
        std::vector<ProductMetadata> productMetadataList;
        productMetadataList.reserve(productInformationList.size());
        for (const auto& pInfoPair : productInformationList)
        {
            productMetadataList.emplace_back(pInfoPair.second);
        }

        SelectedResultsIndexes selectedIndexes = selection.selectProducts(productMetadataList);

        m_selectedSubscriptions.clear();
        for (size_t index : selectedIndexes.selected_subscriptions)
        {
            auto& productMetadata = productMetadataList[index];
            m_selectedSubscriptions.push_back(
                { productMetadata.getLine(), productMetadata.getVersion(), productMetadata.subProducts() });
        }

        for (size_t index : selectedIndexes.selected)
        {
            LOGINFO("Product will be downloaded: " << productMetadataList[index].getLine());
            displayProductTags(productInformationList[index].first);
        }

        for (size_t index : selectedIndexes.notselected)
        {
            auto& productPair = productInformationList[index];
            if (!SULUtils::isSuccess(SU_removeProduct(productPair.first)))
            {
                LOGERROR("Failed to remove product: " << productPair.second.getLine());
            }
        }

        SU_Result ret = SU_Result_invalid;
        if (m_supplementOnly)
        {
            ret = SU_synchroniseSupplements(session());
        }
        else
        {
            ret = SU_synchronise(session());
        }

        if (!SULUtils::isSuccess(ret))
        {
            LOGERROR("Failed to synchronise warehouse: "<<ret);
            SULUtils::displayLogs(session(), m_sulLogs);

            std::string error = m_connectionSetup->getUpdateLocationURL(); // error contains the URL that failed
            // Go through logs looking for failed URL
            for(const auto& sulLog : m_sulLogs)
            {
                // Sample message: [E59264] Cannot locate server for https://d2.sophosupd.com/update/catalogue/sdds.VDB_supp.xml
                /* DOWNLOADFAILED -> IDS_AUERROR_DOWNLOAD "Download of %1 failed from server %2" */
                // error is used as %2, so we want the URL.
                std::string prefix = "[E59264] Cannot locate server for ";
                if (sulLog.find(prefix) == 0)
                {
                    error = sulLog.substr(prefix.size());
                }
            }

            setError(error);
            m_error.status = WarehouseStatus::DOWNLOADFAILED; //
            return;
        }

        std::vector<std::string> missingProducts = selectedIndexes.missing;
        if (!missingProducts.empty())
        {
            std::string listOfSourcesMissing;
            for (const auto& missing : missingProducts)
            {
                LOGWARN("Product missing from warehouse: " << missing);
                if (!listOfSourcesMissing.empty())
                {
                    listOfSourcesMissing += ";";
                }
                listOfSourcesMissing += missing;
            }
            setError("Missing product from warehouse");
            m_error.status = WarehouseStatus::PACKAGESOURCEMISSING;

            m_error.Description = listOfSourcesMissing;
            return;
        }

        m_products.clear();
        bool syncSucceeded = true;
        std::stringstream failedProductErrorMessage;
        failedProductErrorMessage << "Failed to synchronise product:";

        for (auto index : selectedIndexes.selected)
        {
            auto& productPair = productInformationList[index];

            if (!SULUtils::isSuccess(SU_getSynchroniseStatus(productPair.first)))
            {
                syncSucceeded = false;
                failedProductErrorMessage << " '" << productPair.second.getLine() << "'";
            }

            m_products.emplace_back(productPair.first, DownloadedProduct(productPair.second));
        }

        if (!syncSucceeded)
        {
            LOGERROR(failedProductErrorMessage.str());
            SULUtils::displayLogs(session(), m_sulLogs);
            setError(failedProductErrorMessage.str());
            m_error.status = WarehouseStatus::DOWNLOADFAILED;
        }
    }
    void WarehouseRepository::purge() const
    {
        SU_purge(session());
    }
    void WarehouseRepository::distribute()
    {
        assert(m_state == State::Synchronized);
        m_state = State::Distributed;

        for (auto& productPair : m_products)
        {
            std::string distributePath = getProductDistributionPath(productPair.second);
            LOGSUPPORT("Distribution path: " << distributePath);
            distributeProduct(productPair, distributePath);
        }

        if (!SULUtils::isSuccess(SU_distribute(session(), SU_DistributionFlag_AlwaysDistribute)))
        {
            LOGERROR("Failed to distribute products");
            SULUtils::displayLogs(session(), m_sulLogs);
            setError("Failed to distribute products");
            m_error.status = WarehouseStatus ::DOWNLOADFAILED;
        }

        bool distSucceeded = true;
        std::stringstream failedProductErrorMessage;
        failedProductErrorMessage << "Failed to synchronise product:";

        for (auto& product : m_products)
        {
            verifyDistributeProduct(product);

            if (product.second.hasError())
            {
                distSucceeded = false;
                failedProductErrorMessage << " '" << product.second.getLine() << "'";
            }
        }

        if (!distSucceeded)
        {
            LOGERROR(failedProductErrorMessage.str());
            SULUtils::displayLogs(session(), m_sulLogs);
            setError(failedProductErrorMessage.str());
            m_error.status = WarehouseStatus::DOWNLOADFAILED;
        }
        LOGINFO("Products downloaded and synchronized with warehouse.");

    }

    void WarehouseRepository::distributeProduct(
        std::pair<SU_PHandle, DownloadedProduct>& productPair,
        const std::string& distributePath)
    {
        productPair.second.setDistributePath(distributePath);
        const char* empty = "";
        // 0 is passed in as a flag to say we do not want to use the default home folder specified in a component's
        // SDDS-Import file.
        if (!SULUtils::isSuccess(SU_addDistribution(productPair.first, distributePath.c_str(), 0, empty, empty)))
        {
            productPair.second.setError(fetchSulError("Failed to set distribution path"));
        }
    }

    void WarehouseRepository::verifyDistributeProduct(std::pair<SU_PHandle, DownloadedProduct>& productPair)
    {
        std::string distributePath = productPair.second.distributePath();
        auto result = SU_getDistributionStatus(productPair.first, distributePath.c_str());
        LOGDEBUG("DISTRIBUTE status" << result);
        if (!SULUtils::isSuccess(result))
        {
            productPair.second.setError(
                fetchSulError(std::string("Product distribution failed: ") + productPair.second.getLine()));
        }
        else
        {
            productPair.second.setProductHasChanged(result == SU_Result_OK);
        }
    }

    std::vector<DownloadedProduct> WarehouseRepository::getProducts() const
    {
        std::vector<DownloadedProduct> products;
        for (const auto& productPair : m_products)
        {
            products.push_back(productPair.second);
        }
        return products;
    }

    void WarehouseRepository::setError(const std::string& error)
    {
        m_error = fetchSulError(error);

        m_state = State::Failure;
    }

    WarehouseError WarehouseRepository::fetchSulError(const std::string& description) const
    {
        WarehouseError error;
        error.Description = description;
        error.status = WarehouseStatus::UNSPECIFIED;
        if (m_session)
        {
            std::tie(error.status, error.SulError) = getSulCodeAndDescription(session());
        }
        return error;
    }

    void WarehouseRepository::setConnectionSetup(
        const ConnectionSetup& connectionSetup,
        const ConfigurationData& configurationData)
    {
        assert(m_state == State::Initialized);

        m_connectionSetup = std::unique_ptr<ConnectionSetup>(new ConnectionSetup(connectionSetup));

        // general settings
        std::string certificatePath = Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath();
        std::string localWarehouseRepository = configurationData.getLocalWarehouseRepository();
        bool useSlowSupplements = configurationData.getUseSlowSupplements();

        SU_setLoggingLevel(session(), logLevel(configurationData.getLogLevel()));

        LOGSUPPORT("Certificate path: " << certificatePath);
        SU_setCertificatePath(session(), certificatePath.c_str());
        SU_setRequireSHA384(session(), true);

        LOGSUPPORT("WarehouseRepository local repository: " << localWarehouseRepository);
        SU_setLocalRepository(session(), localWarehouseRepository.c_str());
        SU_setUserAgent(session(), "SULDownloader");

        LOGSUPPORT("Use Slow Supplements: " << useSlowSupplements);
        if (useSlowSupplements)
        {
            SU_setUseDelayedSupplementsMode(session(), SU_DelayedSupplementsMode_Slow);
        }
        else
        {
            SU_setUseDelayedSupplementsMode(session(), SU_DelayedSupplementsMode_Normal);
        }

        if (!SULUtils::isSuccess(SU_setUseHttps(session(), true)))
        {
            setError("Failed to enable use HTTPS updating");
            return;
        }

        std::string ssl_cert_path;

        if (connectionSetup.isCacheUpdate())
        {
            ssl_cert_path = Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
        }
        else
        {
            ssl_cert_path = ":system:";
        }

        if (ssl_cert_path != ConfigurationData::DoNotSetSslSystemPath &&
            !SULUtils::isSuccess(SU_setSslCertificatePath(session(), ssl_cert_path.c_str())))
        {
            setError("Failed to set ssl certificate path");
            return;
        }

        if (!SULUtils::isSuccess(SU_setUseSophosCertStore(session(), true)))
        {
            setError("Failed to set Use Sophos certificate store");
            return;
        }

        const auto& updateLocation = connectionSetup.getUpdateLocationURL();
        if (!SULUtils::isSuccess(SU_addSophosLocation(session(), updateLocation.c_str())))
        {
            LOGSUPPORT("Adding Sophos update location failed: " << updateLocation);
            setError("invalid location");
            return;
        }

        std::string updateSource("SOPHOS");

        {
            // Create copy of deobfuscated password, otherwise it will be deleted before SUL uses it
            // This code in separate block to reduce lifetime of deobfuscated password
            Common::ObfuscationImpl::SecureString deobfuscatedPassword =
                connectionSetup.getProxy().getCredentials().getDeobfuscatedPassword();

            std::string proxyUrlToSul = connectionSetup.getProxy().getProxyUrlAsSulRequires();

            if (!SULUtils::isSuccess(SU_addUpdateSource(
                    session(),
                    updateSource.c_str(),
                    connectionSetup.getCredentials().getUsername().c_str(),
                    connectionSetup.getCredentials().getPassword().c_str(),
                    proxyUrlToSul.c_str(),
                    connectionSetup.getProxy().getCredentials().getUsername().c_str(),
                    deobfuscatedPassword.c_str())))
            {
                LOGERROR("Failed to add Update source: " << updateSource);
                setError("Failed to add Update source");
                return;
            }
        }

        if (connectionSetup.isCacheUpdate())
        {
            const std::string& cacheURL = connectionSetup.getUpdateLocationURL();

            std::string updateCacheCustomerLocation = "http://" + cacheURL + "/sophos/customer";

            if (!SULUtils::isSuccess(SU_addSophosLocation(session(), updateCacheCustomerLocation.c_str())))
            {
                LOGSUPPORT("Adding update cache location failed: " << updateCacheCustomerLocation);
                setError("invalid location");
                return;
            }

            std::string redirectAddress = cacheURL + "/sophos/warehouse";

            for (std::string externalURL : { "d1.sophosupd.com/update",
                                             "d1.sophosupd.net/update",
                                             "d2.sophosupd.com/update",
                                             "d2.sophosupd.net/update",
                                             "d3.sophosupd.com/update",
                                             "d3.sophosupd.net/update" })
            {
                if (!SULUtils::isSuccess(SU_addRedirect(session(), externalURL.c_str(), redirectAddress.c_str())))
                {
                    LOGSUPPORT(
                        "Adding update cache re direct failed: " << redirectAddress << ", for : " << externalURL);
                    setError("invalid redirect");
                    return;
                }
            }
        }
    }

    WarehouseRepository::WarehouseRepository()
        : WarehouseRepository(false)
    {}

    void WarehouseRepository::reset()
    {
        m_state = State::Initialized;
        m_session.reset();
        m_products.clear();
        m_error.reset();
        m_connectionSetup.reset();
        m_rootDistributionPath = "";
        m_catalogueInfo.reset();
        m_selectedSubscriptions.clear();
        m_supplementOnly = false;
        // m_sulLogs deliberately don't clear
    }

    WarehouseRepository::WarehouseRepository(bool immediatelyCreateSession) :
        m_state(State::Initialized),
        m_error()
    {
        if (immediatelyCreateSession)
        {
            createSession();
        }
    }

    void WarehouseRepository::createSession()
    {
        m_session.reset(new SULSession());
        if (m_session->m_session == nullptr)
        {
            throw SulDownloaderException("Failed to Initialize Sul");
        }
    }

    SU_Handle WarehouseRepository::session() const
    {
        assert(m_session && m_session->m_session != nullptr);
        return m_session->m_session;
    }

    WarehouseRepository::~WarehouseRepository()
    {
        m_products.clear();
        m_session.reset();
    }

    int WarehouseRepository::logLevel(ConfigurationData::LogLevel logLevel)
    {
        switch (logLevel)
        {
            case ConfigurationData::LogLevel::VERBOSE:
                return SU_LoggingLevel_verbose;
            default:
                return SU_LoggingLevel_important;
        }
    }

    std::string WarehouseRepository::getRootDistributionPath() const { return m_rootDistributionPath; }

    void WarehouseRepository::setRootDistributionPath(const std::string& rootDistributionPath)
    {
        m_rootDistributionPath = rootDistributionPath;
    }

    std::string WarehouseRepository::getSourceURL() const
    {
        if (m_connectionSetup)
        {
            return m_connectionSetup->getUpdateLocationURL();
        }
        else
        {
            return "";
        }
    }

    std::vector<suldownloaderdata::ProductInfo> WarehouseRepository::listInstalledProducts() const
    {
        return CatalogueInfo::calculatedListProducts(getProducts(), m_catalogueInfo);
    }

    std::vector<suldownloaderdata::SubscriptionInfo> WarehouseRepository::listInstalledSubscriptions() const
    {
        return m_selectedSubscriptions;
    }

    std::string WarehouseRepository::getProductDistributionPath(
        const suldownloaderdata::DownloadedProduct& product) const
    {
        return Common::FileSystem::join(m_rootDistributionPath, product.getLine());
    }

} // namespace SulDownloader