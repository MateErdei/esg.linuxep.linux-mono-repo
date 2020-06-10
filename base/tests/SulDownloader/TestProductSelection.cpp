/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

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
    // std::string m_absInstallationPath;
    // std::string m_absCertificatePath;
    // std::string m_absSystemSslPath;
    // std::string m_absCacheUpdatePath;
    // std::string m_primaryPath
    // std::string m_distPath

    std::unique_ptr<Tests::TempDir> m_tempDir;

    void SetUp() override
    {
        m_installRootRelPath = "tmp/sophos-av";
        m_certificateRelPath = "tmp/dev_certificates";
        m_systemSslRelPath = "tmp/etc/ssl/certs";
        m_cacheUpdateSslRelPath = "tmp/etc/cachessl/certs";
        m_primaryPath = "tmp/sophos-av/update/cache/primarywarehouse";
        m_distPath = "tmp/sophos-av/update/cache/primary";
        m_tempDir = Tests::TempDir::makeTempDir();

        m_tempDir->makeDirs(std::vector<std::string>{ m_installRootRelPath,
                                                      m_certificateRelPath,
                                                      m_systemSslRelPath,
                                                      m_systemSslRelPath,
                                                      m_cacheUpdateSslRelPath,
                                                      m_primaryPath,
                                                      m_distPath });

        m_tempDir->createFile(m_certificateRelPath + "/ps_rootca.crt", "empty");
        m_tempDir->createFile(m_certificateRelPath + "/rootca.crt", "empty");

        m_absInstallationPath = m_tempDir->absPath(m_installRootRelPath);
        m_absCertificatePath = m_tempDir->absPath(m_certificateRelPath);
        m_absSystemSslPath = m_tempDir->absPath(m_systemSslRelPath);
        m_absCacheUpdatePath = m_tempDir->absPath(m_cacheUpdateSslRelPath);
    }

    void TearDown() override {}

    enum ProductIdForDev
    {
        Primary_Rec_CORE = 1,
        ProdA_Rec_MDR = 2,
        DiffA_Rec_MDR = 3,
        UNK_Rec_NONE = 4,
        Primary_Rec_CORE_V2 = 5,
        ProdA_PREV_MDR = 6,
        Primary_PREV_CORE = 7,
        ProdB_Rec_SAV = 8,
        CS_BASE_MDR = 9,
        CS_Base_MDR_WITH_FEATURES = 10,
        ServerProtectionForLinux_Base = 11,
        Product_EDR = 12,
        CS_EDR_SAV = 13
    };
    SulDownloader::suldownloaderdata::ProductMetadata createTestProductMetaData(int productItem);
    SulDownloader::suldownloaderdata::SubProducts productAsSubProducts(std::vector<ProductIdForDev> productItems)
    {
        SulDownloader::suldownloaderdata::SubProducts subproducts;
        for (auto devid : productItems)
        {
            auto metadata = createTestProductMetaData(devid);
            subproducts.push_back({ metadata.getLine(), metadata.getVersion() });
        }
        return subproducts;
    }

    std::vector<SulDownloader::suldownloaderdata::ProductMetadata> simulateWarehouseContent(
        const std::vector<ProductIdForDev>& products)
    {
        std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
        for (auto productID : products)
        {
            warehouseProducts.emplace_back(createTestProductMetaData(static_cast<int>(productID)));
        }
        return warehouseProducts;
    }

    bool expect_selected(
        const SulDownloader::suldownloaderdata::ProductMetadata& primaryProduct,
        const ProductSubscription& productSubscription)
    {
        auto configurationData =
            suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

        configurationData.setProductsSubscription({});

        configurationData.setPrimarySubscription(productSubscription);

        auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

        std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
        warehouseProducts.push_back(primaryProduct);

        auto selectedProducts = productSelection.selectProducts(warehouseProducts);

        std::vector<size_t> selected{ 0 };
        return selectedProducts.selected == selected;
    }
};

SulDownloader::suldownloaderdata::ProductMetadata ProductSelectionTest::createTestProductMetaData(int productItem)
{
    SulDownloader::suldownloaderdata::ProductMetadata metadata;
    if (productItem == Primary_Rec_CORE)
    {
        metadata.setLine("BaseProduct-RigidName");
        metadata.setDefaultHomePath("Linux1");
        metadata.setVersion("9.1.1.1");
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security1");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "CORE" });
    }

    if (productItem == ProdA_Rec_MDR)
    {
        metadata.setLine("PrefixOfProduct-SimulateProductA");
        metadata.setDefaultHomePath("Linux2");
        metadata.setVersion("9.1.1.1");
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security2");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "MDR" });
    }

    if (productItem == DiffA_Rec_MDR)
    {
        metadata.setLine("DifferentPrefix-SimulateProductA");
        metadata.setDefaultHomePath("Linux3");
        metadata.setVersion("9.1.1.1");
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security3");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "MDR" });
    }

    if (productItem == UNK_Rec_NONE) // used for missing product test.
    {
        metadata.setLine("Unknown Product");
        metadata.setDefaultHomePath("Linux4");
        metadata.setVersion("9.1.1.1");
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security4");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({});
    }

    // Products with different releaseTag and base version, used for testing keepProduct
    if (productItem == Primary_Rec_CORE_V2)
    {
        metadata.setLine("BaseProduct-RigidName");
        metadata.setDefaultHomePath("Linux1");
        metadata.setVersion("10.1.1.1");
        metadata.setBaseVersion("10");
        metadata.setName("Linux Security1");
        Tag tag("RECOMMENDED", "10", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "CORE" });
    }

    if (productItem == ProdA_PREV_MDR)
    {
        metadata.setLine("PrefixOfProduct-SimulateProductA");
        metadata.setDefaultHomePath("Linux2");
        metadata.setVersion("9.1.1.1");
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security2");
        Tag tag("PREVIEW", "9", "PREVIEW");
        metadata.setTags({ tag });
        metadata.setFeatures({ "MDR" });
    }

    if (productItem == Primary_PREV_CORE)
    {
        metadata.setLine("BaseProduct-RigidName");
        metadata.setDefaultHomePath("Linux1");
        metadata.setVersion("9.1.1.2"); // higher version
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security1");
        Tag tag("PREVIEW", "9", "PREVIEW");
        metadata.setTags({ tag });
        metadata.setFeatures({ "CORE" });
    }

    if (productItem == ProdB_Rec_SAV)
    {
        metadata.setLine("PrefixOfProduct-SimulateProductB");
        metadata.setDefaultHomePath("Linux2");
        metadata.setVersion("9.1.1.1");
        metadata.setBaseVersion("9");
        metadata.setName("Linux Security2");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "SAV" });
    }

    if (productItem == CS_BASE_MDR)
    {
        metadata.setLine("CS_Base_MDR");
        metadata.setVersion("1.1.2");
        metadata.setName("ComponentSuite");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setSubProduts(productAsSubProducts({ Primary_Rec_CORE, ProdA_Rec_MDR }));
    }

    if (productItem == CS_Base_MDR_WITH_FEATURES)
    {
        metadata.setLine("CS_Base_MDR_WITH_FEATURES");
        metadata.setVersion("1.1.2");
        metadata.setName("ComponentSuite_withFeatures");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "CORE" });
        metadata.setSubProduts(productAsSubProducts({ Primary_Rec_CORE, ProdA_Rec_MDR }));
    }

    if (productItem == ServerProtectionForLinux_Base)
    {
        metadata.setLine("ServerProtectionLinux-Base"); // with features, but special case for
        metadata.setVersion("1.1.2");
        metadata.setName("ComponentSuite-WithFeaturesSpecialCase");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "CORE" });
        metadata.setSubProduts(productAsSubProducts({ Primary_Rec_CORE, ProdA_Rec_MDR }));
    }

    if (productItem == Product_EDR)
    {
        metadata.setLine("ServerProtectionForLinux-EDR"); // with features, but special case for
        metadata.setVersion("1.1.2");
        metadata.setName("Simple EDR component");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setFeatures({ "EDR" });
    }

    if (productItem == CS_EDR_SAV)
    {
        metadata.setLine("CS_EDR_SAV");
        metadata.setVersion("1.1.2");
        metadata.setName("ComponentSuite_ForEDR_SAV");
        Tag tag("RECOMMENDED", "9", "RECOMMENDED");
        metadata.setTags({ tag });
        metadata.setSubProduts(productAsSubProducts({ Product_EDR, ProdB_Rec_SAV }));
    }

    return metadata;
}

TEST_F(ProductSelectionTest, CreateProductSelection_SelectingZeroProductsDoesNotThrow) // NOLINT
{
    suldownloaderdata::ConfigurationData configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;

    EXPECT_NO_THROW(productSelection.selectProducts(warehouseProducts)); // NOLINT
}

TEST_F(ProductSelectionTest, CreateProductSelection_SelectProductsShouldReturnAllProductsFound) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, ProdA_Rec_MDR, DiffA_Rec_MDR });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    // only primaryProduct and correctProduct Will be selected
    EXPECT_EQ(selectedProducts.missing.size(), 0);
    std::vector<size_t> notselected{ 2 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0, 1 };
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F( // NOLINT
    ProductSelectionTest,
    CreateProductSelection_SelectProductsShouldReturnMissingAllNonPrefixNamedProducts)
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

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

TEST_F(ProductSelectionTest, MissingSubscriptionsShouldBeReported) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ UNK_Rec_NONE });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{ configurationData.getPrimarySubscription().rigidName(),
                                      configurationData.getProductsSubscription()[0].rigidName() };
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 0 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{};
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F(ProductSelectionTest, MissingSubscriptionsShouldBeReportedForNonPrimaryProducts) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, DiffA_Rec_MDR });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{ configurationData.getProductsSubscription()[0].rigidName() };
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1 }; // DiffA_Rec_MDR
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0 }; // Primary_Rec_Core
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F( // NOLINT
    ProductSelectionTest,
    CreateProductSelection_SelectProductsShouldReturnCorrectProductsWhenWarehouseContainsMoreThanOneVersion)
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    /*
     * primarySubscription: "BaseProduct-RigidName"
     * products: "PrefixOfProduct-SimulateProductA"
     */
    ProductSelection productSelection = ProductSelection::CreateProductSelection(configurationData);

    std::vector<suldownloaderdata::ProductMetadata> warehouseProducts;
    SulDownloader::suldownloaderdata::ProductMetadata metadata1 = createTestProductMetaData(Primary_Rec_CORE);
    SulDownloader::suldownloaderdata::ProductMetadata metadata2 = createTestProductMetaData(ProdA_Rec_MDR);
    SulDownloader::suldownloaderdata::ProductMetadata metadata3 = createTestProductMetaData(DiffA_Rec_MDR);
    SulDownloader::suldownloaderdata::ProductMetadata metadata4 = createTestProductMetaData(UNK_Rec_NONE);
    SulDownloader::suldownloaderdata::ProductMetadata metadata5 = createTestProductMetaData(Primary_Rec_CORE_V2);
    SulDownloader::suldownloaderdata::ProductMetadata metadata6 = createTestProductMetaData(ProdA_PREV_MDR);

    warehouseProducts.push_back(metadata1);
    warehouseProducts.push_back(metadata2);
    warehouseProducts.push_back(metadata3);
    warehouseProducts.push_back(metadata4);
    warehouseProducts.push_back(metadata5);
    warehouseProducts.push_back(metadata6);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    EXPECT_EQ(selectedProducts.missing.size(), 0);
    EXPECT_EQ(selectedProducts.notselected.size(), 3); // 2, 3, 5
    EXPECT_EQ(selectedProducts.selected.size(), 3);    // 0 1 4

    // 0 and 4 differ only by the base version. Hence, both are selected
    std::vector<size_t> selected{ 0, 4, 1 };
    EXPECT_EQ(selectedProducts.selected, selected);

    // match filter line and recommended
    EXPECT_EQ(warehouseProducts[0].getLine(), "BaseProduct-RigidName");

    EXPECT_EQ(warehouseProducts[0].getLine(), warehouseProducts[4].getLine());
    EXPECT_TRUE(warehouseProducts[0].hasTag("RECOMMENDED"));
    EXPECT_TRUE(warehouseProducts[4].hasTag("RECOMMENDED"));

    EXPECT_EQ(selectedProducts.selected[2], 1);
    std::vector<size_t> notselected{ 2, 3, 5 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
}

TEST_F(ProductSelectionTest, ShouldSelectTheCorrectProductWhenMoreThanOneVersionIsAvailable) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, ProdA_PREV_MDR, ProdA_Rec_MDR });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{};
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1 }; // ProdA_PREV_MDR
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0, 2 }; // Primary_Rec_CORE, ProdA_Rec_MDR
    EXPECT_EQ(selectedProducts.selected, selected);

    auto productSubscription = configurationData.getProductsSubscription()[0];
    ProductSubscription newSubscription(
        productSubscription.rigidName(), productSubscription.baseVersion(), "PREVIEW", "");

    configurationData.setProductsSubscription({ newSubscription });

    productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    selectedProducts = productSelection.selectProducts(warehouseProducts);

    EXPECT_EQ(selectedProducts.missing, missing);
    // swap the ProdA_PREV_MDR with ProdA_Rec_MDR
    notselected[0] = 2;
    selected[1] = 1;
    EXPECT_EQ(selectedProducts.notselected, notselected);
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F(ProductSelectionTest, FeaturesShouldFilterTheProductsToBeInstalled) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setFeatures({ "CORE", "SENSORS" }); // no MDR in the features.
    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, ProdA_PREV_MDR, ProdA_Rec_MDR });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{};
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1, 2 }; // ProdA_PREV_MDR and ProdA_Rec_MDR
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0 }; // Primary_Rec_CORE
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F(ProductSelectionTest, ShouldReportMissingIfNoProductSelectedWithCOREFeature) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    // configuring subscription targeting only ProdA_Rec_MDR
    auto currentProductSubscription = configurationData.getProductsSubscription()[0];
    configurationData.setProductsSubscription({});
    configurationData.setPrimarySubscription(currentProductSubscription);

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ ProdA_Rec_MDR });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{ ProductSelection::MissingCoreProduct() };
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{};
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0 }; // ProdA_Rec_MDR
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F(ProductSelectionTest, SelectMainSubscription) // NOLINT
{
    auto wh = simulateWarehouseContent({ Primary_Rec_CORE });
    SulDownloader::suldownloaderdata::ProductMetadata primaryProduct = wh[0];
    ProductSubscription subscription(primaryProduct.getLine(), "", "RECOMMENDED", "");

    // select by recommended tag
    EXPECT_TRUE(expect_selected(primaryProduct, subscription));

    subscription = ProductSubscription(primaryProduct.getLine(), "notbaseversion", "RECOMMENDED", "");
    // select by recommended tag - ignore base version
    EXPECT_TRUE(expect_selected(primaryProduct, subscription));

    SulDownloader::suldownloaderdata::ProductMetadata previewPrimaryProdut = primaryProduct;
    previewPrimaryProdut.setBaseVersion("9");
    previewPrimaryProdut.setTags({ Tag("PREVIEW", "9", "lab") });
    subscription = ProductSubscription(previewPrimaryProdut.getLine(), "9", "PREVIEW", "");
    // select by tag and base version matching
    EXPECT_TRUE(expect_selected(previewPrimaryProdut, subscription));

    subscription = ProductSubscription(primaryProduct.getLine(), "", "", primaryProduct.getVersion());
    // select by version
    EXPECT_TRUE(expect_selected(primaryProduct, subscription));

    // do not select if rigid name is different
    subscription = ProductSubscription(primaryProduct.getLine() + 'b', "", "RECOMMENDED", "");
    EXPECT_FALSE(expect_selected(primaryProduct, subscription));

    // do not select if tag is different
    subscription = ProductSubscription(primaryProduct.getLine(), "", "RECOMMENDED", "");
    EXPECT_FALSE(expect_selected(previewPrimaryProdut, subscription));
}

// Case 1: LINUXEP-7309
/*
 * Case 1: Wh contains 4 products and configuration file requires: Subscription base/recommended, product1/recommended,
 * product2/recommended with features CORE,SAV,MDR selectProducts should select produts 1,3,4. set notselected to 2, and
 * have empty missing.
 */
TEST_F(ProductSelectionTest, ShoudNotSelectPreviewProduct) // NOLINT
{
    auto warehouseProducts =
        simulateWarehouseContent({ Primary_Rec_CORE, Primary_PREV_CORE, ProdB_Rec_SAV, ProdA_Rec_MDR });
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setProductsSubscription(
        { ProductSubscription(warehouseProducts[2].getLine(), "", "RECOMMENDED", ""),
          ProductSubscription(warehouseProducts[3].getLine(), "", "RECOMMENDED", "") });
    configurationData.setFeatures({ "CORE", "SAV", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{};
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0, 2, 3 };
    EXPECT_EQ(selectedProducts.selected, selected);
}

/*
 * Case 2: Wh contains 4 products and configuration file requires: Subscription base/recommended, product1/recommended,
 product2/recommended with features CORE,SAV

    selectProducts should select product 1,3. Set notselected to 2 and 4 and have empty missing.
 */
TEST_F(ProductSelectionTest, ShoudNotSelectProductIfNotINFeatureSet) // NOLINT
{
    auto warehouseProducts =
        simulateWarehouseContent({ Primary_Rec_CORE, Primary_PREV_CORE, ProdB_Rec_SAV, ProdA_Rec_MDR });
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    configurationData.setProductsSubscription(
        { ProductSubscription(warehouseProducts[2].getLine(), "", "RECOMMENDED", ""),
          ProductSubscription(warehouseProducts[3].getLine(), "", "RECOMMENDED", "") });
    configurationData.setFeatures({ "CORE", "SAV" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{};
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1, 3 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0, 2 };
    EXPECT_EQ(selectedProducts.selected, selected);
}

/*
 * Case 3: Wh contains products 1, 3 and configuration file requires: Subscription  base/recommended,
 product2/recommended with features CORE,MDR

   selectProducts should select product 1. Set not selected to 3. Set missing: Product 2.
   */
TEST_F(ProductSelectionTest, ShouldReportMissingProductAndDoNotSelectIfNotInFeature) // NOLINT
{
    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, ProdB_Rec_SAV });
    auto nonWhProducts = simulateWarehouseContent({ ProdA_Rec_MDR });
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setProductsSubscription({
        ProductSubscription(nonWhProducts[0].getLine(), "", "RECOMMENDED", ""),
    });
    configurationData.setFeatures({ "CORE", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{ nonWhProducts[0].getLine() };
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0 };
    EXPECT_EQ(selectedProducts.selected, selected);
}
/*

Case 4: Wh contains products 1, 3 and configuration file requires: Subscription  base/recommended, with features
CORE,MDR

   selectProducts should select product 1. Set not selected to 3. Set missing: to empty. (Note that Feature MDR has been
ignored).
   */
TEST_F(ProductSelectionTest, ShouldNotSetToMissingIFFilteredByFeatureSet) // NOLINT
{
    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, ProdB_Rec_SAV });
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setProductsSubscription({});
    configurationData.setFeatures({ "CORE", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{};
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 1 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 0 };
    EXPECT_EQ(selectedProducts.selected, selected);
}
/*

Case 4: Wh contains products 2,3,4 and configuration file requires: Subscription base/recommended, product1/recommended,
prodcut2/recommended features CORE,SAV,MDR.

   selectProducts should select product 3,4. Set not selected to 2. Set missing to: base/recommended
 */
TEST_F(ProductSelectionTest, ShouldReportIfPrimarySubscriptionNotInWarehouse) // NOLINT
{
    auto warehouseProducts = simulateWarehouseContent({ Primary_PREV_CORE, ProdB_Rec_SAV, ProdA_Rec_MDR });

    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setProductsSubscription(
        { ProductSubscription(warehouseProducts[1].getLine(), "", "RECOMMENDED", ""),
          ProductSubscription(warehouseProducts[2].getLine(), "", "RECOMMENDED", "") });
    configurationData.setFeatures({ "CORE", "SAV", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{ configurationData.getPrimarySubscription().rigidName() };
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 0 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 1, 2 };
    EXPECT_EQ(selectedProducts.selected, selected);
}

/*
Case 4: Wh contains products 2,3,4 and configuration file requires: Subscription product1/recommended,
product2/recommended features CORE,SAV,MDR.

selectProducts should select product 3,4. Set not selected to 2. Set missing to: No Core Product
*/
TEST_F(ProductSelectionTest, ShouldReportMissingIfNoCoreProduct) // NOLINT
{
    auto warehouseProducts = simulateWarehouseContent({ Primary_PREV_CORE, ProdB_Rec_SAV, ProdA_Rec_MDR });

    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setPrimarySubscription(
        ProductSubscription(warehouseProducts[1].getLine(), "", "RECOMMENDED", ""));
    configurationData.setProductsSubscription(
        { ProductSubscription(warehouseProducts[2].getLine(), "", "RECOMMENDED", "") });
    configurationData.setFeatures({ "CORE", "SAV", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{ ProductSelection::MissingCoreProduct() };
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 0 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 1, 2 };
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F(ProductSelectionTest, ShouldAllowSelectionOfNewerVersion) // NOLINT
{
    auto warehouseProducts = simulateWarehouseContent({ Primary_PREV_CORE, Primary_Rec_CORE_V2 });

    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));
    configurationData.setPrimarySubscription(
        ProductSubscription(warehouseProducts[1].getLine(), "", "", warehouseProducts[1].getVersion()));
    configurationData.setProductsSubscription({});
    configurationData.setFeatures({ "CORE", "SAV", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    std::vector<std::string> missing{};
    EXPECT_EQ(selectedProducts.missing, missing);
    std::vector<size_t> notselected{ 0 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 1 };
    EXPECT_EQ(selectedProducts.selected, selected);
    EXPECT_EQ(selectedProducts.selected_subscriptions, selected);
}

TEST_F(ProductSelectionTest, SelectingComponentSuiteWithoutFeatures_ExpandToComponent) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto metadata = createTestProductMetaData(CS_BASE_MDR);
    ProductSubscription productSubscription{ metadata.getLine(), "", "RECOMMENDED", "" };
    configurationData.setProductsSubscription({});
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setFeatures({ "CORE", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    // observe that we have changed the order to make the product that should be installed last the first one in the
    // list. but the CS_BASE_MDR defines the components are first the Primary_Rec_CORE and than ProdA_Rec_MDR
    auto warehouseProducts = simulateWarehouseContent({ ProdA_Rec_MDR, Primary_Rec_CORE, CS_BASE_MDR });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    // although the component
    EXPECT_EQ(selectedProducts.missing.size(), 0);
    // CS_BASE_MDR is not selected as a product to download, but it will be available in the selected_subscriptions
    std::vector<size_t> notselected{ 2 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    // the selected, returns in the correct order that the products should be installed.
    std::vector<size_t> selected{ 1, 0 };
    EXPECT_EQ(selectedProducts.selected, selected);

    std::vector<size_t> selected_subscriptions{ 2 };
    EXPECT_EQ(selectedProducts.selected_subscriptions, selected_subscriptions);
}

TEST_F(
    ProductSelectionTest,
    SelectingComponentSuiteWithoutFeatures_ExpandToComponentAndTheOrderCanBeControlledByTheNameOfTheSubComponents) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto metadata = createTestProductMetaData(CS_BASE_MDR);
    ProductSubscription productSubscription{ metadata.getLine(), "", "RECOMMENDED", "" };
    configurationData.setProductsSubscription({});
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setFeatures({ "CORE", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    // observe that we have changed the order to make the product that should be installed last the first one in the
    // list. but the CS_BASE_MDR defines the components are first the Primary_Rec_CORE and than ProdA_Rec_MDR
    auto warehouseProducts = simulateWarehouseContent({ ProdA_Rec_MDR, Primary_Rec_CORE, CS_BASE_MDR });

    // because the component suite sets the order of the components as Primary_Rec_CORE, ProdA_Rec_MDR
    // That is the way the will be selected (last first)
    auto selectedProducts = productSelection.selectProducts(warehouseProducts);
    std::vector<size_t> selected{ 1, 0 };
    EXPECT_EQ(selectedProducts.selected, selected);

    // but if we were to change the rigidname of ProdA_REC_MDR to CS_Base_MDR_main_comp
    // that will than be installed first.
    // the order is still Primary_Rec_CORE, CS_Base_MDR_maincomp as before,
    // but because the name contains the component suite, it will be bring forward
    auto subproducts = productAsSubProducts({ Primary_Rec_CORE, ProdA_Rec_MDR });
    std::string newRigidName = "CS_Base_MDR_maincomp";
    subproducts[1].m_line = newRigidName;
    warehouseProducts[0].setLine(newRigidName);
    warehouseProducts[2].setSubProduts(subproducts);

    selectedProducts = productSelection.selectProducts(warehouseProducts);
    // see the order has changed to be 0, 1 instead of 1,0
    selected = std::vector<size_t>{ 0, 1 };
    EXPECT_EQ(selectedProducts.selected, selected);
}

TEST_F(ProductSelectionTest, SelectingComponentSuiteWithFeatures_DoesNotExpandToComponent) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto metadata = createTestProductMetaData(CS_Base_MDR_WITH_FEATURES);
    ProductSubscription productSubscription{ metadata.getLine(), "", "RECOMMENDED", "" };
    configurationData.setProductsSubscription({});
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setFeatures({ "CORE", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent({ Primary_Rec_CORE, ProdA_Rec_MDR, CS_Base_MDR_WITH_FEATURES });

    // given that the component suite has feature, only CS_Base_MDR_WITH_FEATURES will be directly selected
    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    // although the component
    EXPECT_EQ(selectedProducts.missing.size(), 0);
    std::vector<size_t> notselected{ 0, 1 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 2 };
    EXPECT_EQ(selectedProducts.selected, selected);

    std::vector<size_t> selected_subscriptions{ 2 };
    EXPECT_EQ(selectedProducts.selected_subscriptions, selected_subscriptions);
}

TEST_F(ProductSelectionTest, SelectingComponentSuiteWithFeaturesButBase_ExpandToComponent) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto metadata = createTestProductMetaData(ServerProtectionForLinux_Base);
    ProductSubscription productSubscription{ metadata.getLine(), "", "RECOMMENDED", "" };
    configurationData.setProductsSubscription({});
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setFeatures({ "CORE", "MDR" });

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);
    auto warehouseProducts =
        simulateWarehouseContent({ ProdA_Rec_MDR, Primary_Rec_CORE, ServerProtectionForLinux_Base });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    // although the component
    EXPECT_EQ(selectedProducts.missing.size(), 0);
    // ServerProtectionForLinux_Base has not been selected to download directly only its subcomponents
    std::vector<size_t> notselected{ 2 };
    EXPECT_EQ(selectedProducts.notselected, notselected);
    std::vector<size_t> selected{ 1, 0 };
    EXPECT_EQ(selectedProducts.selected, selected);

    std::vector<size_t> selected_subscriptions{ 2 };
    EXPECT_EQ(selectedProducts.selected_subscriptions, selected_subscriptions);
}

TEST_F(ProductSelectionTest, ComponentSuitesWithoutAnySelectedProductShouldNotBeReportedAsSelected) // NOLINT
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(ConfigurationDataBase::createJsonString("", ""));

    auto metadata = createTestProductMetaData(CS_BASE_MDR);
    ProductSubscription primarySubscription{ metadata.getLine(), "", "RECOMMENDED", "" };
    configurationData.setPrimarySubscription(primarySubscription);
    metadata = createTestProductMetaData(CS_EDR_SAV);
    ProductSubscription productSubscription{ metadata.getLine(), "", "RECOMMENDED", "" };
    configurationData.setProductsSubscription({ productSubscription });
    configurationData.setFeatures({ "CORE", "MDR" }); // features are not selecting EDR or SAV
    // this test presents two subscriptions targeting 2 component suites:
    //  CS_BASE_MDR and CS_EDR_SAV
    //  but the features will select only products inside:
    //  CS_BASE_MDR
    //  hence, the subscription CS_EDR_SAV should not be counted as 'selected'

    auto productSelection = suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    auto warehouseProducts = simulateWarehouseContent(
        { Primary_Rec_CORE, ProdA_Rec_MDR, CS_BASE_MDR, Product_EDR, ProdB_Rec_SAV, CS_EDR_SAV });

    auto selectedProducts = productSelection.selectProducts(warehouseProducts);

    // although the component
    EXPECT_EQ(selectedProducts.missing.size(), 0);

    // Product_EDR, ProdB_Rec_SAV and CS_EDR_SAV were filtered out due to the FEATURES set
    // CS_BASE_MDR is not marked to be downloaded, only its components.
    std::vector<size_t> notselected{ 2, 3, 4, 5 };
    EXPECT_EQ(selectedProducts.notselected, notselected);

    // Select set refers only to the components, not the component suite: Primary_Rec_CORE, ProdA_Rec_MDR
    std::vector<size_t> selected{ 0, 1 };
    EXPECT_EQ(selectedProducts.selected, selected);

    // Selected subscription here refers only to the CS_BASE_MDR
    std::vector<size_t> selected_subscriptions{ 2 };
    EXPECT_EQ(selectedProducts.selected_subscriptions, selected_subscriptions);
}
