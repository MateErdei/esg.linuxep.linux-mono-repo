/******************************************************************************************************

Copyright 2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gmock/gmock.h"

#include <SulDownloader/suldownloaderdata/ConnectionSetup.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ISdds3Repository.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/RepositoryError.h>

using namespace ::testing;
using namespace SulDownloader;

class MockSdds3Repository : public suldownloaderdata::ISdds3Repository
{
public:
    MOCK_CONST_METHOD0(hasError, bool(void));
    MOCK_CONST_METHOD0(hasImmediateFailError, bool(void));
    MOCK_CONST_METHOD0(getError, suldownloaderdata::RepositoryError(void));
    MOCK_METHOD3(synchronize, bool(const suldownloaderdata::ConfigurationData& configurationData,const suldownloaderdata::ConnectionSetup& connectionSetup,const bool ignoreFailedSupplementRefresh));
    MOCK_METHOD0(distribute, void(void));
    MOCK_CONST_METHOD0(purge, void(void));
    MOCK_METHOD1(setWillInstall, void(const bool willInstall));
    MOCK_CONST_METHOD0(getProducts, std::vector<suldownloaderdata::DownloadedProduct>(void));
    MOCK_CONST_METHOD0(getSourceURL, std::string(void));

    MOCK_CONST_METHOD1(getProductDistributionPath, std::string(const suldownloaderdata::DownloadedProduct&));
    MOCK_CONST_METHOD0(listInstalledProducts, std::vector<suldownloaderdata::ProductInfo>(void));
    MOCK_CONST_METHOD0(
        listInstalledSubscriptions,
        std::vector<suldownloaderdata::SubscriptionInfo>(void));

    MOCK_METHOD3(tryConnect, bool(
        const suldownloaderdata::ConnectionSetup&,
        bool,
        const suldownloaderdata::ConfigurationData&
    ));
};
