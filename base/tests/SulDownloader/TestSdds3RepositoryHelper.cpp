/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TestSdds3RepositoryHelper.h"

using namespace SulDownloader;

void TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator(SulDownloader::RespositoryCreaterFunc creator)
{
    Sdds3RepositoryFactory::instance().replaceCreator(std::move(creator));
}

void TestSdds3RepositoryHelper::restoreSdds3RepositoryFactory()
{
    SulDownloader::Sdds3RepositoryFactory::instance().restoreCreator();
}

::testing::AssertionResult sdds3ProductInfoIsEquivalent(
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

::testing::AssertionResult sdds3ListProductInfoIsEquivalent(
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
        auto ret = sdds3ProductInfoIsEquivalent(m_expr, n_expr, expected[i], resulted[i]);
        if (!ret)
        {
            return ret;
        }
    }
    return ::testing::AssertionSuccess();
}
using namespace SulDownloader;

std::vector<suldownloaderdata::SubscriptionInfo> sdds3SubscriptionsFromProduct(
    const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>& products)
{
    std::vector<suldownloaderdata::SubscriptionInfo> subscriptions;
    for (auto& product : products)
    {
        subscriptions.push_back({ product.getLine(), product.getProductMetadata().getVersion(), {} });
    }
    return subscriptions;
}
