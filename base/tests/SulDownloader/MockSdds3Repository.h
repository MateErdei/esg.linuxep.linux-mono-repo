// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/DownloadReport/ProductInfo.h"
#include "SulDownloader/suldownloaderdata/ConnectionSetup.h"
#include "SulDownloader/suldownloaderdata/DownloadedProduct.h"
#include "SulDownloader/suldownloaderdata/ISdds3Repository.h"
#include "SulDownloader/suldownloaderdata/RepositoryError.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace SulDownloader;

class MockSdds3Repository : public suldownloaderdata::ISdds3Repository
{
public:
    MOCK_CONST_METHOD0(hasError, bool(void));
    MOCK_CONST_METHOD0(hasImmediateFailError, bool(void));
    MOCK_CONST_METHOD0(getError, suldownloaderdata::RepositoryError(void));
    MOCK_METHOD(bool, synchronize, (const Common::Policy::UpdateSettings& updateSettings,const suldownloaderdata::ConnectionSetup& connectionSetup,const bool ignoreFailedSupplementRefresh), (override));
    MOCK_METHOD0(distribute, void(void));
    MOCK_CONST_METHOD0(purge, void(void));
    MOCK_METHOD1(setDoUnpackRepository, void(const bool willInstall));
    MOCK_CONST_METHOD0(getProducts, std::vector<suldownloaderdata::DownloadedProduct>(void));
    MOCK_CONST_METHOD0(getSourceURL, std::string(void));

    MOCK_CONST_METHOD1(getProductDistributionPath, std::string(const suldownloaderdata::DownloadedProduct&));
    MOCK_CONST_METHOD0(listInstalledProducts, std::vector<Common::DownloadReport::ProductInfo>(void));
    MOCK_CONST_METHOD0(
        listInstalledSubscriptions,
        std::vector<suldownloaderdata::SubscriptionInfo>(void));

    MOCK_METHOD(bool, tryConnect, (
        const suldownloaderdata::ConnectionSetup&,
        bool,
        const Common::Policy::UpdateSettings&
    ), (override));
};
