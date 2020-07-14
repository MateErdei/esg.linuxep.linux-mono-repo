/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <SulDownloader/suldownloaderdata/ProductMetadata.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class TestProductMetadata: public LogOffInitializedTests{};
TEST_F(TestProductMetadata, extractSulComponentsWorkForEmptyComponent) // NOLINT
{
    auto subComponents = ProductMetadata::extractSubProductsFromSulSubComponents("", {});
    EXPECT_TRUE(subComponents.empty());
    ProductMetadata p;
    p.subProducts();
}

TEST_F(TestProductMetadata, extractSulComponentShouldProduceValidProductKey)
{
    std::vector<ProductKey> listOfExpectedProductKey = { { "rigidname", "v1" },
                                                         { "rig-gid-name-with-dash", "10.56.890" },
                                                         { "rigid", "" } };
    for (auto& expectedProductKey : listOfExpectedProductKey)
    {
        std::string expectedSulSubComponent = expectedProductKey.m_line + ";" + expectedProductKey.m_version;
        ProductKey calculated = ProductMetadata::extractProductKeyFromSubComponent(expectedSulSubComponent);
        EXPECT_EQ(calculated, expectedProductKey);
    }
}

TEST_F(TestProductMetadata, rigidNameWithDotDashShouldBeConsideredValid)
{
    ProductKey expectedProductKey{ "name-with-;-is-ok", "10.0.5" };
    ProductKey calculated = ProductMetadata::extractProductKeyFromSubComponent("name-with-;-is-ok;10.0.5");
    EXPECT_EQ(calculated, expectedProductKey);
}

TEST_F(TestProductMetadata, invalidCombinationIsRejected)
{
    std::vector<std::string> invalidCombinations = {
        "",        // missing ;
        "-",       // missing ;
        "1290.87", // missing ;
        ";",       // empty rigid name and version is not acceptable
        "invalid"  // missing ;
    };

    for (auto& invalidCombination : invalidCombinations)
    {
        EXPECT_THROW(ProductMetadata::extractProductKeyFromSubComponent(invalidCombination), std::invalid_argument);
    }
}

TEST_F(TestProductMetadata, extractSulComponentsHandleGracefullyInvalidArguments)
{
    std::vector<std::string> entries = { "rig;1.0.0", "invalid", "name2;5.9" };
    SubProducts subProducts = ProductMetadata::extractSubProductsFromSulSubComponents("", entries);

    SubProducts expected{ { "rig", "1.0.0" }, { "name2", "5.9" } };
    EXPECT_EQ(subProducts, expected);
}

TEST_F(TestProductMetadata, ProductMetadataHandleCorrectlySettingSubComponents)
{
    std::vector<std::string> entries = { "rig;1.0.0", "name2;5.9" };
    ProductMetadata productMetadata;
    productMetadata.setSubProduts(ProductMetadata::extractSubProductsFromSulSubComponents("", entries));
    SubProducts subProducts = productMetadata.subProducts();

    SubProducts expected{ { "rig", "1.0.0" }, { "name2", "5.9" } };
    EXPECT_EQ(subProducts, expected);
}

TEST_F(TestProductMetadata, combineSubProductsShouldReturnOwnRigidNameAndVersionForEmptySubComponents)
{
    ProductMetadata productMetadata;
    productMetadata.setLine("line");
    productMetadata.setVersion("1");
    SubProducts subProducts = ProductMetadata::combineSubProducts(std::vector<ProductMetadata>{ productMetadata });

    SubProducts expected{ { "line", "1" } };

    EXPECT_EQ(subProducts, expected);
}

TEST_F(TestProductMetadata, combineSubProductsShouldReturnTheSubComponentsWhenNotEmpty)
{
    ProductMetadata productMetadata;
    productMetadata.setLine("line");
    productMetadata.setVersion("1");
    SubProducts line1subProd{ { "sub1", "2" }, { "sub2", "3" } };
    productMetadata.setSubProduts(line1subProd);
    SubProducts subProducts = ProductMetadata::combineSubProducts(std::vector<ProductMetadata>{ productMetadata });

    SubProducts expected = line1subProd;

    EXPECT_EQ(subProducts, expected);
}

TEST_F(TestProductMetadata, combineSubProductsShouldNotRepeatProducts)
{
    ProductMetadata productMetadata;
    productMetadata.setLine("line");
    productMetadata.setVersion("1");
    SubProducts line1subProd{ { "sub1", "2" }, { "sub2", "3" } };
    productMetadata.setSubProduts(line1subProd);

    ProductMetadata productMetadata2;
    productMetadata2.setLine("line");
    productMetadata2.setVersion("2");
    SubProducts line2subProd{ { "sub3", "2" }, { "sub2", "3" } };
    productMetadata2.setSubProduts(line2subProd);
    SubProducts subProducts =
        ProductMetadata::combineSubProducts(std::vector<ProductMetadata>{ productMetadata, productMetadata2 });

    SubProducts expected{ { "sub1", "2" }, { "sub2", "3" }, { "sub3", "2" } };

    EXPECT_EQ(subProducts, expected);
}
