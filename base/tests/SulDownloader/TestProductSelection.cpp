/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigurationDataBase.h"
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/TempDir.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class ProductSelectionTest : public ConfigurationDataBase
{
public:
    std::string m_installRootRelPath;
    std::string m_certificateRelPath;
    std::string m_systemSslRelPath;
    std::string m_cacheUpdateSslRelPath;

    // come from ConfigurationDataBAse
    //std::string m_absInstallationPath;
    //std::string m_absCertificatePath;
    //std::string m_absSystemSslPath;
    //std::string m_absCacheUpdatePath;
    //std::string m_primaryPath
    //std::string m_distPath

    std::unique_ptr<Tests::TempDir> m_tempDir;

    void SetUp() override
    {
        m_installRootRelPath = "tmp/sophos-av";
        m_certificateRelPath = "tmp/dev_certificates";
        m_systemSslRelPath = "tmp/etc/ssl/certs";
        m_cacheUpdateSslRelPath = "tmp/etc/cachessl/certs";
        m_primaryPath = "tmp/sophos-av/update/cache/primarywarehouse";
        m_distPath =  "tmp/sophos-av/update/cache/primary";
        m_tempDir = Tests::TempDir::makeTempDir();

        m_tempDir->makeDirs(std::vector<std::string>{ m_installRootRelPath,
                                                      m_certificateRelPath,
                                                      m_systemSslRelPath,
                                                      m_systemSslRelPath,
                                                      m_cacheUpdateSslRelPath,
                                                      m_primaryPath,
                                                      m_distPath});

        m_tempDir->createFile(m_certificateRelPath + "/ps_rootca.crt", "empty");
        m_tempDir->createFile(m_certificateRelPath + "/rootca.crt", "empty");

        m_absInstallationPath = m_tempDir->absPath(m_installRootRelPath);
        m_absCertificatePath = m_tempDir->absPath(m_certificateRelPath);
        m_absSystemSslPath = m_tempDir->absPath(m_systemSslRelPath);
        m_absCacheUpdatePath = m_tempDir->absPath(m_cacheUpdateSslRelPath);
    }

    void TearDown() override {}

    std::string createJsonString()
    {
        std::string otherprefix = R"("prefixNames": [
                               "PrefixOfProduct"
                               ],)";
        std::string newprefix = R"("prefixNames": [
                               "DifferentPrefix"
                               ],)";
        return ConfigurationDataBase::createJsonString(otherprefix, newprefix);
    }

    SulDownloader::suldownloaderdata::ProductMetadata createTestProductMetaData(int productItem)
    {
        SulDownloader::suldownloaderdata::ProductMetadata metadata;

        if (productItem == 1)
        {
            metadata.setLine("PrimaryProduct-RigidName");
            metadata.setDefaultHomePath("Linux1");
            metadata.setVersion("9.1.1.1");
            metadata.setBaseVersion("9");
            metadata.setName("Linux Security1");
            Tag tag("RECOMMENDED", "9", "RECOMMENDED");
            metadata.setTags({ tag });
        }

        if (productItem == 2)
        {
            metadata.setLine("PrefixOfProduct-SimulateProductA");
            metadata.setDefaultHomePath("Linux2");
            metadata.setVersion("9.1.1.1");
            metadata.setBaseVersion("9");
            metadata.setName("Linux Security2");
            Tag tag("RECOMMENDED", "9", "RECOMMENDED");
            metadata.setTags({ tag });
        }

        if (productItem == 3)
        {
            metadata.setLine("DifferentPrefix-SimulateProductA");
            metadata.setDefaultHomePath("Linux3");
            metadata.setVersion("9.1.1.1");
            metadata.setBaseVersion("9");
            metadata.setName("Linux Security3");
            Tag tag("RECOMMENDED", "9", "RECOMMENDED");
            metadata.setTags({ tag });
        }

        if (productItem == 4) // used for missing product test.
        {
            metadata.setLine("Unknown Product");
            metadata.setDefaultHomePath("Linux4");
            metadata.setVersion("9.1.1.1");
            metadata.setBaseVersion("9");
            metadata.setName("Linux Security4");
            Tag tag("RECOMMENDED", "9", "RECOMMENDED");
            metadata.setTags({ tag });
        }

        // Products with different releaseTag and base version, used for testing keepProduct
        if (productItem == 5)
        {
            metadata.setLine("PrimaryProduct-RigidName");
            metadata.setDefaultHomePath("Linux1");
            metadata.setVersion("10.1.1.1");
            metadata.setBaseVersion("10");
            metadata.setName("Linux Security1");
            Tag tag("RECOMMENDED", "10", "RECOMMENDED");
            metadata.setTags({ tag });
        }

        if (productItem == 6)
        {
            metadata.setLine("PrefixOfProduct-SimulateProductA");
            metadata.setDefaultHomePath("Linux2");
            metadata.setVersion("9.1.1.1");
            metadata.setBaseVersion("9");
            metadata.setName("Linux Security2");
            Tag tag("PREVIEW", "9", "PREVIEW");
            metadata.setTags({ tag });
        }

        return metadata;
    }
};

TEST_F(ProductSelectionTest, CreateProductSelection_SelectingZeroProductsDoesNotThrow) // NOLINT
{
    suldownloaderdata::ConfigurationData configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString());

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;

    EXPECT_NO_THROW(productSelection.selectProducts(warehouseProducts)); // NOLINT
}

TEST_F(ProductSelectionTest, CreateProductSelection_SelectProductsShouldReturnAllProductsFound) // NOLINT
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString());

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
    SulDownloader::suldownloaderdata::ProductMetadata metadata1 = createTestProductMetaData(1);
    SulDownloader::suldownloaderdata::ProductMetadata metadata2 = createTestProductMetaData(2);
    SulDownloader::suldownloaderdata::ProductMetadata metadata3 = createTestProductMetaData(3);

    warehouseProducts.push_back(metadata1);
    warehouseProducts.push_back(metadata2);
    warehouseProducts.push_back(metadata3);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    EXPECT_EQ(selectedProducts.missing.size(), 0);
    EXPECT_EQ(selectedProducts.notselected.size(), 0);
    EXPECT_EQ(selectedProducts.selected.size(), 3);
}

TEST_F( // NOLINT
    ProductSelectionTest,
    CreateProductSelection_SelectProductsShouldReturnMissingAllNonPrefixNamedProducts)
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString());

    ProductSelection productSelection = ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
    SulDownloader::suldownloaderdata::ProductMetadata metadata = createTestProductMetaData(4);
    warehouseProducts.push_back(metadata);

    // missing warehouse products
    SulDownloader::suldownloaderdata::ProductMetadata metadata1 = createTestProductMetaData(1);
    SulDownloader::suldownloaderdata::ProductMetadata metadata2 = createTestProductMetaData(2);
    SulDownloader::suldownloaderdata::ProductMetadata metadata3 = createTestProductMetaData(3);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    EXPECT_EQ(selectedProducts.missing.size(), 2);
    EXPECT_EQ(selectedProducts.missing[0].c_str(), metadata1.getLine());
    EXPECT_EQ(selectedProducts.missing[1].c_str(), metadata2.getLine());
    EXPECT_EQ(selectedProducts.notselected.size(), 1);
    EXPECT_EQ(selectedProducts.selected.size(), 0);
}

TEST_F(ProductSelectionTest, CreateProductSelection_SelectProductsShouldReturnMixOfSelectedAndMissingProducts) // NOLINT
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString());

    ProductSelection productSelection = ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
    SulDownloader::suldownloaderdata::ProductMetadata metadata1 = createTestProductMetaData(1);
    SulDownloader::suldownloaderdata::ProductMetadata metadata2 = createTestProductMetaData(2);
    SulDownloader::suldownloaderdata::ProductMetadata metadata3 = createTestProductMetaData(3);
    SulDownloader::suldownloaderdata::ProductMetadata metadata4 = createTestProductMetaData(4);

    warehouseProducts.push_back(metadata1);
    warehouseProducts.push_back(metadata3);
    warehouseProducts.push_back(metadata4);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    EXPECT_EQ(selectedProducts.missing.size(), 1);
    EXPECT_EQ(selectedProducts.missing[0].c_str(), metadata2.getLine());
    EXPECT_EQ(selectedProducts.notselected.size(), 1);
    EXPECT_EQ(selectedProducts.selected.size(), 2);
}

TEST_F( // NOLINT
    ProductSelectionTest,
    CreateProductSelection_SelectProductsShouldReturnCorrectProductsWhenWarehouseContainsMoreThanOneVersion)
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString());

    /*
     * FixProductName: "PrimaryProduct-RigidName"
     * FixProductName: "PrefixOfProduct-SimulateProductA"
     * Prefix: "DifferentPrefix"
     */
    ProductSelection productSelection = ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
    SulDownloader::suldownloaderdata::ProductMetadata metadata1 = createTestProductMetaData(1);
    SulDownloader::suldownloaderdata::ProductMetadata metadata2 = createTestProductMetaData(2);
    SulDownloader::suldownloaderdata::ProductMetadata metadata3 = createTestProductMetaData(3);
    SulDownloader::suldownloaderdata::ProductMetadata metadata4 = createTestProductMetaData(4);
    SulDownloader::suldownloaderdata::ProductMetadata metadata5 = createTestProductMetaData(5);
    SulDownloader::suldownloaderdata::ProductMetadata metadata6 = createTestProductMetaData(6);

    warehouseProducts.push_back(metadata1);
    warehouseProducts.push_back(metadata2);
    warehouseProducts.push_back(metadata3);
    warehouseProducts.push_back(metadata4);
    warehouseProducts.push_back(metadata5);
    warehouseProducts.push_back(metadata6);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    EXPECT_EQ(selectedProducts.missing.size(), 0);
    EXPECT_EQ(selectedProducts.notselected.size(), 2); //3, 5
    EXPECT_EQ(selectedProducts.selected.size(), 4); // 0 1 2 4

    // 0 and 4 differ only by the base version. Hence, both are selected
    EXPECT_EQ(selectedProducts.selected[0], 0);
    EXPECT_EQ(selectedProducts.selected[1], 4);
    // match filter line and recommended
    EXPECT_EQ(warehouseProducts[0].getLine(), "PrimaryProduct-RigidName");

    EXPECT_EQ(warehouseProducts[0].getLine(), warehouseProducts[4].getLine());
    EXPECT_TRUE(warehouseProducts[0].hasTag("RECOMMENDED"));
    EXPECT_TRUE(warehouseProducts[4].hasTag("RECOMMENDED"));



    EXPECT_EQ(selectedProducts.selected[2], 1);
    EXPECT_EQ(selectedProducts.selected[3], 2);
    EXPECT_EQ(selectedProducts.notselected[0], 3);
    EXPECT_EQ(selectedProducts.notselected[1], 5);
}
