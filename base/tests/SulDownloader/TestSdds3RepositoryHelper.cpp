// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "TestSdds3RepositoryHelper.h"

namespace SulDownloader
{
    void TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator(SulDownloader::RespositoryCreaterFunc creator)
    {
        Sdds3RepositoryFactory::instance().replaceCreator(std::move(creator));
    }

    void TestSdds3RepositoryHelper::restoreSdds3RepositoryFactory()
    {
        Sdds3RepositoryFactory::instance().restoreCreator();
    }

    ::testing::AssertionResult productInfoIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const Common::DownloadReport::ProductInfo& expected,
        const Common::DownloadReport::ProductInfo& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";
        if (expected.m_productName != resulted.m_productName)
        {
            return ::testing::AssertionFailure()
                   << s.str() << "product name differs. Expected: " << expected.m_productName
                   << " received: " << resulted.m_productName;
        }
        if (expected.m_version != resulted.m_version)
        {
            return ::testing::AssertionFailure() << s.str() << "version differs. Expected: " << expected.m_version
                                                 << " received: " << resulted.m_version;
        }
        if (expected.m_rigidName != resulted.m_rigidName)
        {
            return ::testing::AssertionFailure()
                   << s.str() << "product rigidname differs. Expected: " << expected.m_rigidName
                   << " received: " << resulted.m_rigidName;
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult listProductInfoIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const std::vector<Common::DownloadReport::ProductInfo>& expected,
        const std::vector<Common::DownloadReport::ProductInfo>& resulted)
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

    std::vector<suldownloaderdata::SubscriptionInfo> subscriptionsFromProduct(
        const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>& products)
    {
        std::vector<suldownloaderdata::SubscriptionInfo> subscriptions;
        for (auto& product : products)
        {
            subscriptions.push_back({ product.getLine(), product.getProductMetadata().getVersion(), {} });
        }
        return subscriptions;
    }
} // namespace SulDownloader
