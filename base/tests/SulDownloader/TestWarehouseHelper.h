/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <memory>

namespace SulDownloader
{
    class TestWarehouseHelper
    {
    public:
        void replaceWarehouseCreator(
            std::function<suldownloaderdata::IWarehouseRepositoryPtr(const suldownloaderdata::ConfigurationData&)>
                creator);
        void restoreWarehouseFactory();
    };
    ::testing::AssertionResult productInfoIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const suldownloaderdata::ProductInfo& expected,
        const suldownloaderdata::ProductInfo& resulted);
    ::testing::AssertionResult listProductInfoIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const std::vector<suldownloaderdata::ProductInfo>& expected,
        const std::vector<suldownloaderdata::ProductInfo>& resulted);
    std::vector<suldownloaderdata::SubscriptionInfo> subscriptionsFromProduct(
        const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>& products);

} // namespace SulDownloader
