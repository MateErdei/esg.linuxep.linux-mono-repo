//
// Created by pair on 06/06/18.
//

#include <sys/stat.h>
#include "Warehouse.h"
#include "Product.h"
#include "Tag.h"
#include "ProductSelection.h"
#include "SULUtils.h"
#include "SULRaii.h"
#include "Logger.h"
#include <cassert>

namespace
{
    static std::vector<SulDownloader::Tag> getTags(SU_PHandle &product)
    {
        std::vector<SulDownloader::Tag> tags;
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

    bool hasError( const std::vector<std::pair<SU_PHandle, SulDownloader::Product>> & products)
    {
        if (products.empty())
        {
            return false;
        }
        for ( const auto & product : products)
        {
            if (product.second.hasError())
            {
                return true;
            }
        }
        return false;
    }
}

namespace SulDownloader
{

    std::unique_ptr<Warehouse> Warehouse::FetchConnectedWarehouse(const ConfigurationData &configurationData)
    {

        ConnectionSelector connectionSelector;
        auto candidates = connectionSelector.getConnectionCandidates(configurationData);

        for ( auto & connectionSetup : candidates)
        {
            auto warehouse = std::unique_ptr<Warehouse>(new Warehouse(true));
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
            return warehouse;
        }

        auto warehouse_empty = std::unique_ptr<Warehouse>(new Warehouse(false));
        warehouse_empty->setError("Failed to connect to warehouse");
        return warehouse_empty;
    }

    bool Warehouse::hasError() const
    {
        return !m_error.Description.empty() || ::hasError(m_products);
    }

    WarehouseError Warehouse::getError() const
    {
        return m_error;
    }


    void Warehouse::synchronize(ProductSelection & selection)
    {
        assert( m_state == State::Connected);
        m_state = State::Synchronized;

        std::vector<std::pair<SU_PHandle, ProductInformation>> productInformationList;

        // create the ProductInformation for all the entries in the warehouse
        while (true)
        {

            SU_PHandle product = SU_getProductRelease(session());

            if (product == nullptr)
            {
                break;
            }

            ProductInformation productInformation;

            std::string line = SulQueryProductMetadata(product, "R_Line", 0);
            std::string baseVersion = SulQueryProductMetadata(product, "VersionId", 0);
            std::vector<Tag> tags(getTags(product));
            productInformation.setName(line);

            productInformation.setTags(tags);

            productInformation.setVersion(baseVersion);
            productInformationList.emplace_back(product, productInformation);
        }


        std::vector<std::pair<SU_PHandle, ProductInformation>> selectedProducts;
        std::vector<std::pair<SU_PHandle, ProductInformation>> unwantedProducts;
        for ( auto & productPair : productInformationList)
        {
            if ( selection.keepProduct(productPair.second))
            {
                LOGSUPPORT("Product will be downloaded: " << productPair.second.getName());
                selectedProducts.push_back(productPair);
            }
            else
            {
                unwantedProducts.push_back(productPair);
            }
        }

        for ( auto & productPair : unwantedProducts)
        {
            if(!SULUtils::isSuccess(SU_removeProduct(productPair.first)))
            {
                SULUtils::displayLogs(session());
                LOGERROR("Failed to remove product: " << productPair.second.getName());
            }
        }


        if(!SULUtils::isSuccess(SU_synchronise(session())))
        {
            LOGERROR("Failed to synchronise warehouse");
            setError("Failed to Sync warehouse");
            return;
        }

        SULUtils::displayLogs(session());

        std::vector<ProductInformation> selectedProdInfo;
        for( auto pInfoPair : selectedProducts)
        {
            selectedProdInfo.push_back(pInfoPair.second);
        }

        std::vector<std::string> missingProducts = selection.missingProduct(selectedProdInfo);
        if ( !missingProducts.empty())
        {
            for( const auto & missing: missingProducts)
            {
                LOGSUPPORT("Product missing from warehouse: " << missing);
            }
            setError("Missing product from warehouse");
            m_error.status = WarehouseStatus::PACKAGESOURCEMISSING;
            return;
        }



        m_products.clear();
        for (auto & productPair : selectedProducts)
        {

            if(!SULUtils::isSuccess(SU_getSynchroniseStatus(productPair.first)))
            {
                LOGERROR("Failed to synchronise product: " << productPair.second.getName());
            }

            m_products.emplace_back(productPair.first, Product(productPair.second));
        }
    }


    void Warehouse::distribute()
    {
        assert( m_state == State::Synchronized);
        m_state = State::Distributed;

        for ( auto & productPair : m_products)
        {
            auto  & product = productPair.second;
            std::string distributePath = "/tmp/distribute/" + product.distributionFolderName();
            LOGSUPPORT("Distribution path: " << distributePath);
            distributeProduct(productPair, distributePath);
        }

        if(!SULUtils::isSuccess(SU_distribute(session(), SU_DistributionFlag_AlwaysDistribute)))
        {
            LOGERROR("Failed to distribute products");
            setError("Failed to distribute products");
        }


        for (auto & product : m_products)
        {
            verifyDistributeProduct(product);
        }
    }

    void Warehouse::distributeProduct(std::pair<SU_PHandle, Product> &productPair, const std::string &distributePath)
    {
        productPair.second.setDistributePath(distributePath) ;
        const char *empty = "";
        if ( !SULUtils::isSuccess(SU_addDistribution(productPair.first, distributePath.c_str(),
                                                     SU_AddDistributionFlag_UseDefaultHomeFolder, empty,
                                                     empty)))
        {
            productPair.second.setError( fetchSulError( "Failed to set distribution path"));

        }

    }

    void Warehouse::verifyDistributeProduct(std::pair<SU_PHandle, Product> &productPair)
    {
        std::string distributePath = productPair.second.distributePath();
        auto result = SU_getDistributionStatus(productPair.first,  distributePath.c_str());
        LOGDEBUG("DISTRIBUTE status" << result);
        if (! SULUtils::isSuccess(result))
        {
            SULUtils::displayLogs(session());
            productPair.second.setError( fetchSulError( std::string("Product distribution failed: ") + productPair.second.getName()));
        }
        else
        {
            productPair.second.setProductHasChanged( result == SU_Result_OK );
        }
    }


    std::vector<Product> Warehouse::getProducts() const
    {
        std::vector<Product> products;
        for ( auto & productPair : m_products)
        {
            products.push_back(productPair.second);
        }
        return products;
    }


    void Warehouse::setError(const std::string & error)
    {
        SULUtils::displayLogs(session());
        m_state = State::Failure;

        m_error = fetchSulError(error);
    }

    WarehouseError Warehouse::fetchSulError( const std::string &description) const
    {
        WarehouseError error;
        error.Description = description;
        error.status = WarehouseStatus::UNSPECIFIED;
        if ( session())
        {
            std::tie( error.status, error.SulError) = getSulCodeAndDescription(session());
        }
        return error;

    }


    void Warehouse::setConnectionSetup(const ConnectionSetup & connectionSetup, const ConfigurationData & configurationData)
    {
        assert( m_state == State::Initialized);

        m_connectionSetup = std::unique_ptr<ConnectionSetup>(new ConnectionSetup(connectionSetup));

        // general settings
        std::string certificatePath = configurationData.getCertificatePath();
        std::string localRepository = configurationData.getLocalRepository();

        SU_setLoggingLevel(session(), logLevel( configurationData.getLogLevel()));
        //SU_setLoggingLevel(warehouse->session(), SU_LoggingLevel_important);
        LOGSUPPORT("Certificate path: " << certificatePath);
        SU_setCertificatePath(session(), certificatePath.c_str());

        LOGSUPPORT("Warehouse local repository: " << localRepository);
        SU_setLocalRepository(session(), localRepository.c_str());
        SU_setUserAgent(session(), "SULDownloader");

        bool https = connectionSetup.useHTTPS(); // default will always download over https.

        if(!SULUtils::isSuccess(SU_setUseHttps(session(), https)))
        {
            setError("Failed to enable use HTTPS updating");
            return;
        }

        if (https)
        {
            std::string ssl_cert_path( configurationData.getSSLCertificatePath());

            if(!SULUtils::isSuccess(SU_setSslCertificatePath(session(), ssl_cert_path.c_str())))
            {
                setError("Failed to set ssl certificate path");
                return;
            }

            if(!SULUtils::isSuccess(SU_setUseSophosCertStore(session(), true)))
            {
                setError("Failed to set Use Sophos certificate store");
                return;
            }
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
            std::string cacheURL = connectionSetup.getUpdateLocationURL();
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

    Warehouse::Warehouse(bool createSession ) : m_error(), m_products(), m_session(), m_connectionSetup()
    {
        m_state = State::Initialized;
        if ( createSession )
        {
            m_session.reset(new SULSession());
            if ( m_session->m_session == nullptr)
            {
                setError("Failed to Initialize Sul");
            }
        }


    }

    SU_Handle Warehouse::session() const
    {
        return m_session->m_session;
    }

    Warehouse::~Warehouse()
    {
        m_products.clear();
        m_session.reset();
    }

    int Warehouse::logLevel(ConfigurationData::LogLevel logLevel) {
        switch (logLevel)
        {
            case ConfigurationData::LogLevel::VERBOSE:
                return SU_LoggingLevel_verbose;
            default:
                return SU_LoggingLevel_normal;
        }

    }



}