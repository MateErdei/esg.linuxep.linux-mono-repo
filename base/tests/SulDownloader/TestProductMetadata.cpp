/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <SulDownloader/suldownloaderdata/ProductMetadata.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>


using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

void verifyEquality( const ProductKey& calculated, const ProductKey & expectedProductKey)
{
    EXPECT_EQ(calculated.m_line, expectedProductKey.m_line);
    EXPECT_EQ(calculated.m_version, expectedProductKey.m_version);
}


TEST(TestProductMetadata, extractSulComponentsWorkForEmptyComponent) // NOLINT
{
    auto subComponents = ProductMetadata::extractSubProductsFromSulSubComponents({});
    EXPECT_TRUE(subComponents.empty());
    ProductMetadata p;
    p.subProducts();
}

TEST( TestProductMetadata, extractSulComponentShouldProduceValidProductKey)
{
    std::vector<ProductKey> listOfExpectedProductKey = {
            {"rigidname", "v1"},
            {"rig-gid-name-with-dash", "10.56.890"},
            {"rigid",""}
    };
    for( auto & expectedProductKey: listOfExpectedProductKey)
    {
        std::string expectedSulSubComponent = expectedProductKey.m_line + ";" + expectedProductKey.m_version;
        ProductKey calculated = ProductMetadata::extractProductKeyFromSubComponent(expectedSulSubComponent);
        verifyEquality(calculated,expectedProductKey);
    }
}

TEST( TestProductMetadata, rigidNameWithDotDashShouldBeConsideredValid)
{
    ProductKey expectedProductKey{ "name-with-;-is-ok","10.0.5"};
    ProductKey calculated = ProductMetadata::extractProductKeyFromSubComponent("name-with-;-is-ok;10.0.5");
    verifyEquality(calculated,expectedProductKey);
}


TEST( TestProductMetadata, invalidCombinationIsRejected)
{

    std::vector<std::string> invalidCombinations = {
            "",  // missing ;
            "-", // missing ;
            "1290.87", // missing ;
            ";",    // empty rigid name and version is not acceptable
            "invalid" // missing ;
    };

    for( auto & invalidCombination: invalidCombinations)
    {
        EXPECT_THROW(ProductMetadata::extractProductKeyFromSubComponent(invalidCombination), std::invalid_argument);
    }
}

TEST( TestProductMetadata, extractSulComponentsHandleGracefullyInvalidArguments)
{
    std::vector<std::string> entries = {"rig;1.0.0","invalid","name2;5.9"};
    SubProducts subProducts = ProductMetadata::extractSubProductsFromSulSubComponents(entries);
    EXPECT_EQ( subProducts.size(), 2 );
    verifyEquality( subProducts[0], {"rig", "1.0.0"});
    verifyEquality( subProducts[1], {"name2", "5.9"});
}

TEST( TestProductMetadata, ProductMetadataHandleCorrectlySettingSubComponents)
{

    std::vector<std::string> entries = {"rig;1.0.0","name2;5.9"};
    ProductMetadata productMetadata;
    productMetadata.setSubProduts(ProductMetadata::extractSubProductsFromSulSubComponents(entries));
    SubProducts subProducts = productMetadata.subProducts();
    EXPECT_EQ( subProducts.size(), 2 );
    verifyEquality( subProducts[0], {"rig", "1.0.0"});
    verifyEquality( subProducts[1], {"name2", "5.9"});
}

