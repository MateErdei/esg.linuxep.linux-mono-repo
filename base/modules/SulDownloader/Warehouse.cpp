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
            SU_setCertificatePath(warehouse->session(), certificatePath.c_str());

            SU_setLocalRepository(warehouse->session(), localRepository.c_str());
            SU_setUserAgent(warehouse->session(), "SULDownloader");

            warehouse->setConnectionSetup(connectionSetup);
            if ( warehouse->hasError())
            {
                continue; //TODO deal with the errors
            }

            SU_readRemoteMetadata(warehouse->session());

            return warehouse;
        }

        auto warehouse_empty = std::unique_ptr<Warehouse>(new Warehouse(false));
        warehouse_empty->setError("Failed to connect to the warehouse");
        return warehouse_empty;
    }

    bool Warehouse::hasError() const
    {
        return false;
    }

    const std::string &Warehouse::getError() const
    {
        return m_error;
    }


    void Warehouse::synchronize(ProductSelection & selection)
    {

        std::vector<ProductInformation> productInformationList;

        // create the ProductInformation for all the entries in the warehouse
        while (true)
        {

            SU_PHandle product = SU_getProductRelease(session());
            SULUtils::displayLogs(session());
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
                selectedProducts.push_back(productInformation);
            }
            else
            {
                unwantedProducts.push_back(productInformation);
            }
        }

        for ( auto & productInformation : unwantedProducts)
        {
            SU_removeProduct(productInformation.getPHandle());
        }
        SULUtils::displayLogs(session());
        SU_synchronise(session());
        SULUtils::displayLogs(session());
//        result = SU_synchronise(session);
//        if (!isSuccess(result))
//        {
//            displayLogs(session);
//            ASSERT_TRUE(isSuccess(result));
//        }


        m_products.clear();
        for (auto & product : selectedProducts)
        {

            SU_getSynchroniseStatus(product.getPHandle());
            //TODO: handle the error
            m_products.push_back(Product(product));
            //result =
            //ASSERT_TRUE(isSuccess(result));
        }
    }


    void Warehouse::distribute()
    {
        for ( auto & product : getProducts())
        {
            const char *empty = "";
            std::string distributePath = "/tmp/distribute/" + product.distributionFolderName();
            product.setDistributePath(distributePath);
        }

        SU_distribute(session(), SU_DistributionFlag_AlwaysDistribute);


        for (auto product : getProducts())
        {
            product.getDistributionStatus();
        }
    }

    std::vector<Product> &Warehouse::getProducts()
    {
        return m_products;
    }

    void Warehouse::setError(const std::string & error)
    {
        m_error = error;
    }

    void Warehouse::setConnectionSetup(const ConnectionSetup & connectionSetup)
    {
        for(const auto & sophosUpdateUrl : connectionSetup.getSophosLocationURL())
        {
            SU_addSophosLocation(session(), sophosUpdateUrl.c_str());
        }

        SU_addUpdateSource(session(),
                           "SOPHOS",
                           connectionSetup.getCredentials().getUsername().c_str(),
                           connectionSetup.getCredentials().getPassword().c_str(),
                           connectionSetup.getProxy().getUrl().c_str(),
                           connectionSetup.getProxy().getCredentials().getUsername().c_str(),
                           connectionSetup.getProxy().getCredentials().getPassword().c_str());

//
//        const std::string source = "SOPHOS";
////    const std::string username = "QA940267";
////    const std::string password = "54m5ung";
//        const std::string username = "administrator";
//        const std::string password = "password";
//
////    result = SU_addSophosLocation(session, "http://dci.sophosupd.com/update/");
////    ASSERT_TRUE(isSuccess(result));
////    result = SU_addSophosLocation(session, "http://dci.sophosupd.net/update/");
////    ASSERT_TRUE(isSuccess(result));
//        //result = SU_addSophosLocation(session, "http://localhost:3333/customer_files.live");
//        //result = SU_addSophosLocation(session, "http://ostia.eng.sophos/latest/Virt-vShield/1/74/174e6bde8263d4b72cbe69dff029e62a.dat");
//        result = SU_addSophosLocation(session, "http://ostia.eng.sophos/latest/Virt-vShield");
//
//        ASSERT_TRUE(isSuccess(result));
//
//        result = SU_addUpdateSource(session,
//                                    source.c_str(),
//                                    username.c_str(),
//                                    password.c_str(),
//                                    "",
//                                    "",
//                                    "");
//        ASSERT_TRUE(isSuccess(result));
//
//        std::string localRepository(safegetcwd() + "/" + LOCAL_REPOSITORY);
//        P("Local Repository = " << localRepository);
//        mkdir(localRepository.c_str(), 0700);
//        result = SU_setLocalRepository(session, localRepository.c_str());
//        ASSERT_TRUE(isSuccess(result));
//
//        result = SU_setUserAgent(session, "SULDownloader");
//        ASSERT_TRUE(isSuccess(result));
//
//        result = SU_readRemoteMetadata(session);

    }

    Warehouse::Warehouse(bool createSession ) : m_session()
    {
        if ( createSession )
        {
            m_session.reset(new SULSession());
        }

    }

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