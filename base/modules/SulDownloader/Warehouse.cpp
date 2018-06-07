//
// Created by pair on 06/06/18.
//

extern "C" {
#include <SUL.h>
}

#include <sys/stat.h>
#include "Warehouse.h"
#include "Product.h"
#include "Tag.h"
#include "ProductSelection.h"
#include "SULUtils.h"
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
}

namespace SulDownloader
{

    std::unique_ptr<Warehouse> Warehouse::FetchConnectedWarehouse(const ConfigurationData &configurationData)
    {
        std::string certificatePath = configurationData.getCertificatePath();
        std::string localRepository = configurationData.getLocalRepository();
        ::mkdir(localRepository.c_str(), 0700);//FIXME
        ConnectionSelector connectionSelector;
        auto candidates = connectionSelector.getConnectionCandidates(configurationData);

        for ( auto & connectionSetup : candidates)
        {
            auto warehouse = std::unique_ptr<Warehouse>(new Warehouse(true));
            SU_setLoggingLevel(warehouse->session(), SU_LoggingLevel_verbose);
            //SU_setLoggingLevel(warehouse->session(), SU_LoggingLevel_important);
            LOGSUPPORT("Certificate path: " << certificatePath);
            SU_setCertificatePath(warehouse->session(), certificatePath.c_str());

            LOGSUPPORT("Warehouse local repository: " << localRepository);
            SU_setLocalRepository(warehouse->session(), localRepository.c_str());
            SU_setUserAgent(warehouse->session(), "SULDownloader");

            warehouse->setConnectionSetup(connectionSetup);
            if ( warehouse->hasError())
            {
                continue;
            }

            if (!SULUtils::isSuccess(SU_readRemoteMetadata(warehouse->session())))
            {
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
        return !m_error.Description.empty();
    }

    Error Warehouse::getError() const
    {
        return m_error;
    }


    void Warehouse::synchronize(ProductSelection & selection)
    {
        assert( m_state == State::Connected);
        m_state = State::Synchronized;

        std::vector<ProductInformation> productInformationList;

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

            productInformation.setPHandle(product);

            productInformation.setBaseVersion(baseVersion);

            m_products.emplace_back(productInformation);

        }


        std::vector<ProductInformation> selectedProducts;
        std::vector<ProductInformation> unwantedProducts;
        for ( auto & productInformation : productInformationList)
        {
            if ( selection.keepProduct(productInformation))
            {
                LOGSUPPORT("Product will be downloaded: " << productInformation.getName());
                selectedProducts.push_back(productInformation);
            }
            else
            {
                unwantedProducts.push_back(productInformation);
            }
        }

        for ( auto & productInformation : unwantedProducts)
        {
            if(!SULUtils::isSuccess(SU_removeProduct(productInformation.getPHandle())))
            {
                SULUtils::displayLogs(session());
                LOGERROR("Failed to remove product: " << productInformation.getName());
            }
        }


        if(!SULUtils::isSuccess(SU_synchronise(session())))
        {
            LOGERROR("Failed to synchronise warehouse");
            setError("Failed to Sync warehouse");
        }

        SULUtils::displayLogs(session());



        m_products.clear();
        for (auto & product : selectedProducts)
        {

            if(!SULUtils::isSuccess(SU_getSynchroniseStatus(product.getPHandle())))
            {
                LOGERROR("Failed to synchronise product: " << product.getName());
            }

            m_products.push_back(Product(product));

        }
    }


    void Warehouse::distribute()
    {
        assert( m_state == State::Synchronized);
        m_state = State::Distributed;

        for ( auto & product : getProducts())
        {
            std::string distributePath = "/tmp/distribute/" + product.distributionFolderName();
            LOGSUPPORT("Distribution path: " << distributePath);
            product.setDistributePath(distributePath);
        }

        if(!SULUtils::isSuccess(SU_distribute(session(), SU_DistributionFlag_AlwaysDistribute)))
        {
            LOGERROR("Failed to distribute products");
            setError("Failed to distribute products");
        }


        for (auto product : getProducts())
        {
            product.verifyDistributionStatus();
        }
    }

    std::vector<Product> &Warehouse::getProducts()
    {
        assert( m_state == State::Synchronized || m_state == State::Distributed);
        return m_products;
    }

    void Warehouse::setError(const std::string & error)
    {
        m_state = State::Failure;

        m_error.Description = error;
        m_error.SulCode = Error::NoSulError;
        if ( m_session)
        {
            auto result = SU_getLastError(session());
            if ( !SULUtils::isSuccess(result) )
            {
                m_error.SulCode = result;
                m_error.SulError = SulGetErrorDetails(session());
            }
        }
    }

    void Warehouse::setConnectionSetup(const ConnectionSetup & connectionSetup)
    {
        assert( m_state == State::Initialized);

        m_connectionSetup = std::unique_ptr<ConnectionSetup>(new ConnectionSetup(connectionSetup));


        auto & updateLocation =  connectionSetup.getUpdateLocationURL();
        if(!SULUtils::isSuccess(SU_addSophosLocation(session(), updateLocation.c_str())))
        {
            LOGSUPPORT ("Adding Sophos update location failed: " << updateLocation);
            setError("invalid location"); //
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
                                             "d3.sophosupd.net/update"})
            {
                SU_addRedirect(session(), externalURL.c_str(), cacheURL.c_str());
            }
        }

    }

    Warehouse::Warehouse(bool createSession ) : m_error(), m_products(), m_session(), m_connectionSetup()
    {
        if ( createSession )
        {
            m_session.reset(new SULSession());
        }
        m_state = State::Initialized;

    }        SU_Handle session();

    SU_Handle Warehouse::session()
    {
        return m_session->m_session;
    }

    Warehouse::~Warehouse()
    {
        m_products.clear();
        m_session.reset();
    }
}