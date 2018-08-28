/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "WarehouseRepository.h"

#include "SULUtils.h"
#include "SULRaii.h"
#include "Logger.h"

#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>

#include <cassert>
#include <sstream>

using namespace SulDownloader::suldownloaderdata;

namespace
{
    using TagVector = std::vector<Tag>;

    TagVector getTags(SU_PHandle &product)
    {
        TagVector tags;
        int index = 0;

        while (true)
        {
            std::string tag = SulDownloader::SulQueryProductMetadata(product, "R_ReleaseTagsTag", index);
            if(tag.empty())
            {
                break;
            }

            std::string baseversion = SulDownloader::SulQueryProductMetadata(product, "R_ReleaseTagsBaseVersion", index);
            std::string label = SulDownloader::SulQueryProductMetadata(product, "R_ReleaseTagsLabel", index);

            tags.emplace_back(tag, baseversion, label);
            index++;
        }
        return tags;
    }

    bool hasError( const std::vector<std::pair<SU_PHandle, SulDownloader::suldownloaderdata::DownloadedProduct>> & products)
    {

        for ( const auto & product : products)
        {
            if (product.second.hasError())
            {
                return true;
            }
        }
        return false;
    }

    void displayProductTags(SU_PHandle product, const std::vector<std::string> & attributes)
    {
        LOGDEBUG("\nNew Product");
        for( auto & attribute : attributes)
        {
            LOGDEBUG("Tag: " << attribute << " value: " << SulDownloader::SulQueryProductMetadata(product, attribute, 0));
        }
        LOGDEBUG("\n");
    }


    void displayProductTags( SU_PHandle product)
    {
        displayProductTags(product, {"Line", "VersionId", "Name",
                                     "PublicationTime", "DefaultHomeFolder", "Features",
                                     "Roles", "TargetTypes", "ReleaseTagsBaseVersion",
                                     "ReleaseTags", "Platforms", "Lifestage",
                                     "SAVLine", "ResubscriptionsLine", "Resubscriptions",
                                     "ResubscriptionsVersion"});
    }

}

namespace SulDownloader
{

    std::unique_ptr<WarehouseRepository> WarehouseRepository::FetchConnectedWarehouse(const ConfigurationData &configurationData)
    {
        ConnectionSelector connectionSelector;
        auto candidates = connectionSelector.getConnectionCandidates(configurationData);

        for ( auto & connectionSetup : candidates)
        {
            auto warehouse = std::unique_ptr<WarehouseRepository>(new WarehouseRepository(true));
            if ( warehouse->hasError())
            {
                continue;
            }

            warehouse->setConnectionSetup(connectionSetup, configurationData);

            if ( warehouse->hasError())
            {
                SULUtils::displayLogs(warehouse->session());
                continue;
            }

            if (!SULUtils::isSuccess(SU_readRemoteMetadata(warehouse->session())))
            {
                SULUtils::displayLogs(warehouse->session());
                LOGSUPPORT("Failed to connect to warehouse\n" << warehouse->m_connectionSetup->toString());
                continue;
            }
            // for verbose it will list the entries in the warehouse
            SULUtils::displayLogs(warehouse->session());
            warehouse->m_state = State::Connected;

            // store values from configuration data for later use.
            warehouse->setRootDistributionPath(configurationData.getLocalDistributionRepository());


            return warehouse;
        }
        LOGERROR("Failed to connect to the warehouse");
        auto warehouseEmpty = std::unique_ptr<WarehouseRepository>(new WarehouseRepository(false));
        warehouseEmpty->setError("Failed to connect to warehouse");
        return warehouseEmpty;
    }

    bool WarehouseRepository::hasError() const
    {
        return !m_error.Description.empty() || ::hasError(m_products);
    }

    WarehouseError WarehouseRepository::getError() const
    {
        return m_error;
    }


    void WarehouseRepository::synchronize(ProductSelection & selection)
    {
        assert( m_state == State::Connected);
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
#ifndef NDEBUG
            displayProductTags(product);
#endif
            ProductMetadata productInformation;

            std::string line = SulQueryProductMetadata(product, "R_Line", 0);
            std::string name = SulQueryProductMetadata(product, "Name", 0);
            std::string baseVersion = SulQueryProductMetadata(product, "VersionId", 0);
            std::string defaultHomePath = SulQueryProductMetadata(product, "DefaultHomeFolder", 0);
            std::vector<Tag> tags(getTags(product));

            productInformation.setLine(line);
            productInformation.setName(name);
            productInformation.setTags(tags);
            productInformation.setVersion(baseVersion);
            productInformation.setDefaultHomePath(defaultHomePath);

            productInformationList.emplace_back(product, productInformation);
        }
        std::vector<ProductMetadata> productMetadataList;
        for( auto pInfoPair : productInformationList)
        {
            productMetadataList.push_back(pInfoPair.second);
        }

        SelectedResultsIndexes selectedIndexes = selection.selectProducts(productMetadataList);

        for ( int index : selectedIndexes.selected)
        {
            LOGSUPPORT("Product will be downloaded: " << productMetadataList[index].getLine());
        }

        for (  int index : selectedIndexes.notselected)
        {
            auto & productPair = productInformationList[index];
            if(!SULUtils::isSuccess(SU_removeProduct(productPair.first)))
            {
                SULUtils::displayLogs(session());
                LOGERROR("Failed to remove product: " << productPair.second.getLine());
            }
        }

        if(!SULUtils::isSuccess(SU_synchronise(session())))
        {
            LOGERROR("Failed to synchronise warehouse");
            setError("Failed to Sync warehouse");
            m_error.status = WarehouseStatus::DOWNLOADFAILED;
            return;
        }

        SULUtils::displayLogs(session());

        std::vector<std::string> missingProducts = selectedIndexes.missing;
        if ( !missingProducts.empty())
        {
            std::string listOfSourcesMissing;
            for( const auto & missing: missingProducts)
            {
                LOGSUPPORT("Product missing from warehouse: " << missing);
                if ( !listOfSourcesMissing.empty())
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

        for (int index : selectedIndexes.selected)
        {
            auto & productPair = productInformationList[index];

            if(!SULUtils::isSuccess(SU_getSynchroniseStatus(productPair.first)))
            {
                syncSucceeded = false;
                failedProductErrorMessage << " '" << productPair.second.getLine() << "'";
            }

            m_products.emplace_back(productPair.first, DownloadedProduct(productPair.second));
        }

        if (!syncSucceeded)
        {
            LOGERROR(failedProductErrorMessage.str());
            setError(failedProductErrorMessage.str());
            m_error.status = WarehouseStatus::DOWNLOADFAILED;
        }
    }


    void WarehouseRepository::distribute()
    {
        assert( m_state == State::Synchronized);
        m_state = State::Distributed;

        for ( auto & productPair : m_products)
        {
            std::string distributePath = getRootDistributionPath();

            LOGSUPPORT("Distribution path: " << distributePath);
            distributeProduct(productPair, distributePath);
        }

        if(!SULUtils::isSuccess(SU_distribute(session(), SU_DistributionFlag_AlwaysDistribute)))
        {
            LOGERROR("Failed to distribute products");
            setError("Failed to distribute products");
            m_error.status = WarehouseStatus ::DOWNLOADFAILED;
        }


        bool distSucceeded = true;
        std::stringstream failedProductErrorMessage;
        failedProductErrorMessage << "Failed to synchronise product:";

        for (auto & product : m_products)
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
            setError(failedProductErrorMessage.str());
            m_error.status = WarehouseStatus::DOWNLOADFAILED;
        }

        SULUtils::displayLogs(session());
    }

    void WarehouseRepository::distributeProduct(std::pair<SU_PHandle, DownloadedProduct> &productPair, const std::string &distributePath)
    {
        productPair.second.setDistributePath(distributePath) ;
        const char *empty = "";
        if ( !SULUtils::isSuccess(SU_addDistribution(productPair.first, distributePath.c_str(),
                                                     SU_AddDistributionFlag_UseDefaultHomeFolder, empty,
                                                     empty)))
        {
            SULUtils::displayLogs(session());
            productPair.second.setError( fetchSulError( "Failed to set distribution path"));

        }

    }

    void WarehouseRepository::verifyDistributeProduct(std::pair<SU_PHandle, DownloadedProduct> &productPair)
    {
        std::string distributePath = productPair.second.distributePath();
        auto result = SU_getDistributionStatus(productPair.first,  distributePath.c_str());
        LOGDEBUG("DISTRIBUTE status" << result);
        if (! SULUtils::isSuccess(result))
        {
            SULUtils::displayLogs(session());
            productPair.second.setError( fetchSulError( std::string("Product distribution failed: ") + productPair.second.getLine()));
        }
        else
        {
            productPair.second.setProductHasChanged( result == SU_Result_OK );
        }
    }


    std::vector<DownloadedProduct> WarehouseRepository::getProducts() const
    {
        std::vector<DownloadedProduct> products;
        for ( auto & productPair : m_products)
        {
            products.push_back(productPair.second);
        }
        return products;
    }


    void WarehouseRepository::setError(const std::string & error)
    {
        if ( m_session)
        {
            SULUtils::displayLogs(session());
        }

        m_error = fetchSulError(error);

        m_state = State::Failure;
    }

    WarehouseError WarehouseRepository::fetchSulError( const std::string &description) const
    {
        WarehouseError error;
        error.Description = description;
        error.status = WarehouseStatus::UNSPECIFIED;
        if ( m_session )
        {
            std::tie( error.status, error.SulError) = getSulCodeAndDescription(session());
        }
        return error;

    }


    void WarehouseRepository::setConnectionSetup(const ConnectionSetup & connectionSetup, const ConfigurationData & configurationData)
    {
        assert( m_state == State::Initialized);

        m_connectionSetup = std::unique_ptr<ConnectionSetup>(new ConnectionSetup(connectionSetup));

        // general settings
        std::string certificatePath = configurationData.getCertificatePath();
        std::string localWarehouseRepository = configurationData.getLocalWarehouseRepository();

        SU_setLoggingLevel(session(), logLevel( configurationData.getLogLevel()));

        LOGSUPPORT("Certificate path: " << certificatePath);
        SU_setCertificatePath(session(), certificatePath.c_str());
        SU_setRequireSHA384(session(), true);
        
        LOGSUPPORT("WarehouseRepository local repository: " << localWarehouseRepository);
        SU_setLocalRepository(session(), localWarehouseRepository.c_str());
        SU_setUserAgent(session(), "SULDownloader");

        if(!SULUtils::isSuccess(SU_setUseHttps(session(), true)))
        {
            setError("Failed to enable use HTTPS updating");
            return;
        }

        std::string ssl_cert_path;

        if(connectionSetup.isCacheUpdate())
        {
            ssl_cert_path = configurationData.getUpdateCacheSslCertificatePath();
        }
        else
        {
            ssl_cert_path = configurationData.getSystemSslCertificatePath();
        }
        if (ssl_cert_path != ConfigurationData::DoNotSetSslSystemPath &&
            !SULUtils::isSuccess(SU_setSslCertificatePath(session(), ssl_cert_path.c_str())))
        {
            setError("Failed to set ssl certificate path");
            return;
        }

        if(!SULUtils::isSuccess(SU_setUseSophosCertStore(session(), true)))
        {
            setError("Failed to set Use Sophos certificate store");
            return;
        }

        auto & updateLocation =  connectionSetup.getUpdateLocationURL();
        if(!SULUtils::isSuccess(SU_addSophosLocation(session(), updateLocation.c_str())))
        {
            LOGSUPPORT ("Adding Sophos update location failed: " << updateLocation);
            setError("invalid location");
            return;
        }

        std::string updateSource("SOPHOS");

        if(!SULUtils::isSuccess(SU_addUpdateSource(session(),
                           updateSource.c_str(),
                           connectionSetup.getCredentials().getUsername().c_str(),
                           connectionSetup.getCredentials().getPassword().c_str(),
                           connectionSetup.getProxy().getUrl().c_str(),
                           connectionSetup.getProxy().getCredentials().getUsername().c_str(),
                           connectionSetup.getProxy().getCredentials().getPassword().c_str())))
        {
            LOGERROR("Failed to add Update source: " << updateSource);
            setError("Failed to add Update source");
            return;
        }

        if ( connectionSetup.isCacheUpdate())
        {
            const std::string& cacheURL = connectionSetup.getUpdateLocationURL();
            for (std::string externalURL : {"d1.sophosupd.com/update",
                                            "d1.sophosupd.net/update",
                                            "d2.sophosupd.com/update",
                                            "d2.sophosupd.net/update",
                                            "d3.sophosupd.com/update",
                                            "d3.sophosupd.net/update"}) {
                SU_addRedirect(session(), externalURL.c_str(), cacheURL.c_str());
            }
        }
    }

    WarehouseRepository::WarehouseRepository(bool createSession ) : m_error(), m_products(), m_session(), m_connectionSetup()
    {
        m_state = State::Initialized;
        if ( createSession )
        {
            m_session.reset(new SULSession());
            if ( m_session->m_session == nullptr)
            {
                throw SulDownloaderException("Failed to Initialize Sul");
            }
        }
    }

    SU_Handle WarehouseRepository::session() const
    {
        assert( m_session && m_session->m_session != nullptr );
        return m_session->m_session;
    }

    WarehouseRepository::~WarehouseRepository()
    {
        m_products.clear();
        m_session.reset();
    }

    int WarehouseRepository::logLevel(ConfigurationData::LogLevel logLevel) {
        switch (logLevel)
        {
            case ConfigurationData::LogLevel::VERBOSE:
                return SU_LoggingLevel_verbose;
            default:
                return SU_LoggingLevel_important;
        }

    }

    std::string WarehouseRepository::getRootDistributionPath() const
    {
        return m_rootDistributionPath;
    }

    void WarehouseRepository::setRootDistributionPath(const std::string &rootDistributionPath)
    {
        m_rootDistributionPath = rootDistributionPath;
    }

    std::string WarehouseRepository::getSourceURL() const
    {
        if( m_connectionSetup)
        {
            return m_connectionSetup->getUpdateLocationURL();
        }
        else
        {
            return "";
        }

    }


}