/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TestWarehouseHelper.h"

#include "SulDownloader/WarehouseRepositoryFactory.h"

void SulDownloader::TestWarehouseHelper::replaceWarehouseCreator(
    std::function<
        suldownloaderdata::IWarehouseRepositoryPtr(const SulDownloader::suldownloaderdata::ConfigurationData&)> creator)
{
    SulDownloader::WarehouseRepositoryFactory::instance().replaceCreator(std::move(creator));
}

void SulDownloader::TestWarehouseHelper::restoreWarehouseFactory()
{
    SulDownloader::WarehouseRepositoryFactory::instance().restoreCreator();
}

::testing::AssertionResult SulDownloader::productInfoIsEquivalent(
    const char* m_expr,
    const char* n_expr,
    const suldownloaderdata::ProductInfo& expected,
    const suldownloaderdata::ProductInfo& resulted)
{
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";
    if (expected.m_productName != resulted.m_productName)
    {
        return ::testing::AssertionFailure() << s.str() << "product name differs. Expected: " << expected.m_productName
                                             << " received: " << resulted.m_productName;
    }
    if (expected.m_version != resulted.m_version)
    {
        return ::testing::AssertionFailure()
               << s.str() << "version differs. Expected: " << expected.m_version << " received: " << resulted.m_version;
    }
    if (expected.m_rigidName != resulted.m_rigidName)
    {
        return ::testing::AssertionFailure()
               << s.str() << "product rigidname differs. Expected: " << expected.m_rigidName
               << " received: " << resulted.m_rigidName;
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult SulDownloader::listProductInfoIsEquivalent(
    const char* m_expr,
    const char* n_expr,
    const std::vector<suldownloaderdata::ProductInfo>& expected,
    const std::vector<suldownloaderdata::ProductInfo>& resulted)
{
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";
    if (expected.size() != resulted.size())
    {
        return ::testing::AssertionFailure() << s.str() << " Expected " << expected.size()
                                             << " elements. Received: " << resulted.size() << " elements.";
    }
    for (size_t i = 0; i < expected.size(); i++)
    {
        auto ret = productInfoIsEquivalent(m_expr, n_expr, expected[i], resulted[i]);
        if (!ret)
        {
            return ret;
        }
    }
    return ::testing::AssertionSuccess();
}
using namespace SulDownloader;
std::vector<suldownloaderdata::SubscriptionInfo> SulDownloader::subscriptionsFromProduct(
    const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>& products)
{
    std::vector<suldownloaderdata::SubscriptionInfo> subscriptions;
    for (auto& product : products)
    {
        subscriptions.push_back({ product.getLine(), product.getProductMetadata().getVersion(), {} });
    }
    return subscriptions;
}
