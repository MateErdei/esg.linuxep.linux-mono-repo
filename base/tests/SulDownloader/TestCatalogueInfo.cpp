/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestWarehouseHelper.h"

#include <SulDownloader/suldownloaderdata/ProductMetadata.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <modules/SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <modules/SulDownloader/suldownloaderdata/IWarehouseRepository.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class TestCatalogueInfo : public ::testing::Test
{
public:
    enum class ProdId
    {
        SIMPLE_COMPONENT,
        COMPONENT_SUITE,
        SUB_COMPONENT1,
        SUB_COMPONENT2
    };
    TestCatalogueInfo()
    {
        ProductInfo auxInfo;
        auxInfo = getProductInfo(ProdId::SIMPLE_COMPONENT);
        m_warehouseProductsMetadata.push_back(setupMetadata(auxInfo, {}));
        ProductInfo auxInfoSC1 = getProductInfo(ProdId::SUB_COMPONENT1);
        m_warehouseProductsMetadata.push_back(setupMetadata(auxInfoSC1, {}));
        ProductInfo auxInfoSC2 = getProductInfo(ProdId::SUB_COMPONENT2);
        m_warehouseProductsMetadata.push_back(setupMetadata(auxInfoSC2, {}));
        auxInfo = getProductInfo(ProdId::COMPONENT_SUITE);
        m_warehouseProductsMetadata.push_back(setupMetadata(auxInfo, { auxInfoSC1, auxInfoSC2 }));

        for (auto& pm : m_warehouseProductsMetadata)
        {
            m_catalogueInfo.addInfo(pm.getLine(), pm.getVersion(), pm.getName());
        }
    }

    ProductMetadata setupMetadata(const ProductInfo& pInfo, const std::vector<ProductInfo>& subComp)
    {
        ProductMetadata pm;
        pm.setLine(pInfo.m_rigidName);
        pm.setName(pInfo.m_productName);
        pm.setVersion(pInfo.m_version);
        if (!subComp.empty())
        {
            std::vector<suldownloaderdata::ProductKey> subComps;
            for (auto& subPInfo : subComp)
            {
                subComps.push_back(ProductKey{ .m_line = subPInfo.m_rigidName, .m_version = subPInfo.m_version });
            }
            pm.setSubProduts(subComps);
        }
        return pm;
    }

    std::vector<suldownloaderdata::ProductMetadata> m_warehouseProductsMetadata;
    suldownloaderdata::CatalogueInfo m_catalogueInfo;

    suldownloaderdata::ProductInfo getProductInfo(ProdId prodId)
    {
        using suldownloaderdata::ProductInfo;
        switch (prodId)
        {
            case ProdId::SIMPLE_COMPONENT:
                return ProductInfo{ "simple-rname", "simple-prod", "1s" };
            case ProdId::COMPONENT_SUITE:
                return ProductInfo{ "comp-rname", "comp-name", "1c" };
            case ProdId::SUB_COMPONENT1:
                return ProductInfo{ "sub1-rname", "sub1-name", "1sc" };
            case ProdId::SUB_COMPONENT2:
                return ProductInfo{ "sub2-rname", "sub2-name", "2sc" };
            default:
                return ProductInfo{};
        }
    }

    std::vector<suldownloaderdata::ProductInfo> getProductInfos(const std::vector<ProdId>& indices)
    {
        std::vector<suldownloaderdata::ProductInfo> products;
        for (auto& indice : indices)
        {
            products.emplace_back(getProductInfo(indice));
        }
        return products;
    }

    std::vector<suldownloaderdata::DownloadedProduct> simulateDownloadedProducts(const std::vector<ProdId>& indices)
    {
        std::vector<suldownloaderdata::DownloadedProduct> downloadedProducts;
        for (auto& indice : indices)
        {
            ProductInfo productInfo = getProductInfo(indice);
            for (auto& pm : m_warehouseProductsMetadata)
            {
                if (pm.getLine() == productInfo.m_rigidName && pm.getVersion() == productInfo.m_version)
                {
                    suldownloaderdata::DownloadedProduct downloadedProduct{ pm };
                    downloadedProducts.push_back(downloadedProduct);
                }
            }
        }
        return downloadedProducts;
    }
    std::vector<suldownloaderdata::ProductInfo> simulateAlgorithmOfGettingProductInfo(
        const std::vector<ProdId>& indices)
    {
        std::vector<suldownloaderdata::DownloadedProduct> downloaded = simulateDownloadedProducts(indices);
        return CatalogueInfo::calculatedListProducts(downloaded, m_catalogueInfo);
    }
};

TEST_F(TestCatalogueInfo, calculatedListProducts_simpleComponentDownloadedShouldBeListed) // NOLINT
{
    auto expected = getProductInfos({ ProdId::SIMPLE_COMPONENT });
    auto calculated = simulateAlgorithmOfGettingProductInfo({ ProdId::SIMPLE_COMPONENT });
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, calculated);
}

TEST_F(TestCatalogueInfo, calculatedListProducts_componentSuiteShouldListOnlySubComponents) // NOLINT
{
    auto expected = getProductInfos({ ProdId::SUB_COMPONENT1, ProdId::SUB_COMPONENT2 });
    auto calculated = simulateAlgorithmOfGettingProductInfo({ ProdId::COMPONENT_SUITE });
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, calculated);
}

TEST_F(TestCatalogueInfo, calculatedListProducts_shouldCombineAllTheProducts) // NOLINT
{
    auto expected = getProductInfos({ ProdId::SIMPLE_COMPONENT, ProdId::SUB_COMPONENT1, ProdId::SUB_COMPONENT2 });
    auto calculated = simulateAlgorithmOfGettingProductInfo({ ProdId::SIMPLE_COMPONENT, ProdId::COMPONENT_SUITE });
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, calculated);
}