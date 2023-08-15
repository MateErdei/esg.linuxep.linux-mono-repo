// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once
#include "SulDownloader/suldownloaderdata/DownloadReport.h"

#include <string>

using SulDownloader::suldownloaderdata::RepositoryStatus;

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        static const std::string StartTimeTest{ "20180812 10:00:00" };
        static const std::string FinishTimeTest{ "20180812 11:00:00" };
        static const std::string PreviousStartTime{ "20180811 10:00:00" };
        static const std::string PreviousFinishTime{ "20180811 11:00:00" };
        static const std::string PreviousPreviousStartTime{ "20180810 10:00:00" };
        static const std::string PreviousPreviousFinishTime{ "20180810 11:00:00" };
        static const std::string SophosURL{ "http://sophos.net/update" };

        class DownloadReportTestBuilder
        {
        public:
            enum class UseTime
            {
                PreviousPrevious,
                Previous,
                Later
            };

            static SulDownloader::suldownloaderdata::ProductReport upgradedBaseProduct()
            {
                SulDownloader::suldownloaderdata::ProductReport report;
                report.name = "BaseName";
                report.rigidName = "BaseRigidName";
                report.downloadedVersion = "0.5.0";
                report.installedVersion = "0.5.0";
                report.errorDescription.clear();
                report.productStatus = suldownloaderdata::ProductReport::ProductStatus::Upgraded;
                return report;
            }

            static SulDownloader::suldownloaderdata::ProductReport upgradedPlugin()
            {
                SulDownloader::suldownloaderdata::ProductReport report;
                report.name = "PluginName";
                report.rigidName = "PluginRigidName";
                report.downloadedVersion = "0.5.0";
                report.installedVersion = "0.5.0";
                report.errorDescription.clear();
                report.productStatus = suldownloaderdata::ProductReport::ProductStatus::Upgraded;
                return report;
            }

            static std::vector<SulDownloader::suldownloaderdata::ProductReport> goodProducts()
            {
                std::vector<SulDownloader::suldownloaderdata::ProductReport> products;
                products.push_back(upgradedBaseProduct());
                products.push_back(upgradedPlugin());
                return products;
            }

            static SulDownloader::suldownloaderdata::DownloadReport goodReport(
                UseTime useTime = UseTime::Later,
                bool upgraded = true,
                const std::string& sourceURL = "")
            {
                SulDownloader::suldownloaderdata::DownloadReport report;
                report.m_status = RepositoryStatus::SUCCESS;
                if (sourceURL.empty())
                {
                    report.m_urlSource = SophosURL;
                }
                else
                {
                    report.m_urlSource = sourceURL;
                }

                report.m_description = "";
                report.m_sulError = "";
                switch (useTime)
                {
                    case UseTime::Later:
                        report.m_startTime = StartTimeTest;
                        report.m_finishedTime = FinishTimeTest;
                        break;
                    case UseTime::Previous:
                        report.m_startTime = PreviousStartTime;
                        report.m_finishedTime = PreviousFinishTime;
                        break;
                    case UseTime::PreviousPrevious:
                        report.m_startTime = PreviousPreviousStartTime;
                        report.m_finishedTime = PreviousPreviousFinishTime;
                        break;
                }
                report.m_sync_time = report.m_finishedTime;
                report.m_productReport = goodProducts();
                if (!upgraded)
                {
                    for (auto& product : report.m_productReport)
                    {
                        product.productStatus = ProductReport::ProductStatus::UpToDate;
                    }
                }

                ProductReport baseMetadata = report.m_productReport[0];
                ProductInfo productInfo;
                productInfo.m_productName = baseMetadata.name;
                productInfo.m_version = baseMetadata.downloadedVersion;
                productInfo.m_installedVersion = baseMetadata.installedVersion;
                productInfo.m_rigidName = baseMetadata.rigidName;
                report.m_repositoryComponents.push_back(productInfo);

                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport goodReportWithSubscriptions()
            {
                auto report = goodReport();
                std::vector<ProductReport> products;
                ProductReport compSuite;
                compSuite.rigidName = "ComponentSuite";
                compSuite.downloadedVersion = "v1";
                compSuite.installedVersion="0.5.0";
                products.push_back(compSuite);
                report.m_productReport = products;
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport goodReportWithBaseRigidNameSubscriptions()
            {
                auto report = goodReport();
                std::vector<ProductReport> products;
                ProductReport compSuite;
                compSuite.rigidName = "BaseRigidName";
                compSuite.downloadedVersion = "v1";
                compSuite.installedVersion="0.5.0";
                products.push_back(compSuite);
                report.m_productReport = products;
                return report;
            }

            static std::string goodReportString(UseTime useTime)
            {
                auto report = goodReport(useTime);
                return SulDownloader::suldownloaderdata::DownloadReport::fromReport(report);
            }

            static SulDownloader::suldownloaderdata::DownloadReport badReport(
                UseTime useTime,
                RepositoryStatus status,
                std::string errorDescription)
            {
                SulDownloader::suldownloaderdata::DownloadReport report;
                report.m_status = status;
                report.m_description = std::move(errorDescription);
                report.m_urlSource = SophosURL;
                report.m_sulError = "";
                if (useTime == UseTime::Later)
                {
                    report.m_startTime = StartTimeTest;
                    report.m_finishedTime = FinishTimeTest;
                }
                else
                {
                    report.m_startTime = PreviousStartTime;
                    report.m_finishedTime = PreviousFinishTime;
                }
                report.m_sync_time = report.m_finishedTime;

                if (status != RepositoryStatus::CONNECTIONERROR)
                {
                    report.m_productReport = goodProducts();
                    for (auto& product : report.m_productReport)
                    {
                        product.productStatus =
                            SulDownloader::suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                    }
                }
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport getPluginFailedToInstallReport(UseTime useTime)
            {
                auto report = badReport(useTime, RepositoryStatus::INSTALLFAILED, "");
                report.m_productReport[0].productStatus = suldownloaderdata::ProductReport::ProductStatus::Upgraded;
                report.m_productReport[1].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[1].errorDescription = "Plugin failed to install";
                return report;
            }

            static std::string getPluginFailedToInstallReportString(UseTime useTime)
            {
                SulDownloader::suldownloaderdata::DownloadReport report = getPluginFailedToInstallReport(useTime);
                return SulDownloader::suldownloaderdata::DownloadReport::fromReport(report);
            }

            static void setReportNoUpgrade(SulDownloader::suldownloaderdata::DownloadReport* report)
            {
                for (auto& product : report->m_productReport)
                {
                    product.productStatus = suldownloaderdata::ProductReport::ProductStatus::UpToDate;
                }
            }

            static SulDownloader::suldownloaderdata::DownloadReport installFailedTwoProducts()
            {
                auto report = badReport(UseTime::Later, RepositoryStatus::INSTALLFAILED, "");
                report.m_productReport[0].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[1].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[0].errorDescription = "Base failed to install";
                report.m_productReport[1].errorDescription = "Plugin failed to install";
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport installCaughtError()
            {
                auto report = badReport(UseTime::Later, RepositoryStatus::UNSPECIFIED, "Error associated to install");
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport downloadFailedError()
            {
                auto report = badReport(UseTime::Later, RepositoryStatus::DOWNLOADFAILED, "https://my.update.site/");
                // When we get a download failure, we can't tell which product had the failure.
                // To match this the unit test also doesn't put errorDescription in the product reports.
                report.m_productReport[0].productStatus = suldownloaderdata::ProductReport::ProductStatus::UpToDate;
                report.m_productReport[1].productStatus = suldownloaderdata::ProductReport::ProductStatus::UpToDate;
                report.m_productReport[0].errorDescription = "";
                report.m_productReport[1].errorDescription = "";

                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport baseProductMissing()
            {
                auto report = badReport(UseTime::Later, RepositoryStatus::PACKAGESOURCEMISSING, "BaseName");

                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport bothProductsMissing()
            {
                auto report = badReport(UseTime::Later, RepositoryStatus::PACKAGESOURCEMISSING, "BaseName;PluginName");

                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport connectionError()
            {
                auto report = badReport(UseTime::Later, RepositoryStatus::CONNECTIONERROR, "Failed to synchronize");
                return report;
            }
        };
    } // namespace suldownloaderdata
} // namespace SulDownloader
