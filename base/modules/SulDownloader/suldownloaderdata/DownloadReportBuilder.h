// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IRepository.h"
#include "TimeTracker.h"

#include "Common/DownloadReport/DownloadReport.h"
#include "Common/DownloadReport/RepositoryStatus.h"

namespace SulDownloader::suldownloaderdata
{
    class DownloadReportBuilder
    {
    public:
        enum class VerifyState
        {
            VerifyFailed,
            VerifyCorrect
        };

        static std::vector<Common::DownloadReport::ProductReport> combineProductsAndSubscriptions(
            const std::vector<suldownloaderdata::DownloadedProduct>&,
            const std::vector<suldownloaderdata::SubscriptionInfo>&,
            const Common::DownloadReport::RepositoryStatus& repositoryStatus);

        static Common::DownloadReport::DownloadReport Report(const IRepository&, const TimeTracker& timeTracker);

        static Common::DownloadReport::DownloadReport Report(
            const std::string& sourceURL,
            const std::vector<suldownloaderdata::DownloadedProduct>& products,
            const std::vector<Common::DownloadReport::ProductInfo>& components,
            const std::vector<suldownloaderdata::SubscriptionInfo>& subscriptionsToALCStatus,
            TimeTracker* timeTracker,
            VerifyState verify,
            bool supplementOnly = false,
            bool baseDowngrade = false);

        static Common::DownloadReport::DownloadReport Report(const std::string& errorDescription);

    private:
        static std::string getInstalledVersion(const std::string& rigidName);
        static const std::vector<Common::DownloadReport::ProductInfo> updateRepositoryComponentInstalledVersion(
            const std::vector<Common::DownloadReport::ProductInfo>& repositoryComponents);
    };
} // namespace SulDownloader::suldownloaderdata
