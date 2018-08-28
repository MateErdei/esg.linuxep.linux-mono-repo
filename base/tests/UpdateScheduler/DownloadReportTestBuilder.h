/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <string>

namespace  SulDownloader
{
    namespace suldownloaderdata
    {
        static const std::string StartTimeTest{"20180812 10:00:00"};
        static const std::string FinishTimeTest{"20180812 11:00:00"};
        static const std::string PreviousStartTime{"20180811 10:00:00"};
        static const std::string PreviousFinishTime{"20180811 11:00:00"};
        static const std::string PreviousPreviousStartTime{"20180810 10:00:00"};
        static const std::string PreviousPreviousFinishTime{"20180810 11:00:00"};
        static const std::string SophosURL{"http://sophos.net/update"};

        class DownloadReportTestBuilder
        {
        public:
            enum class UseTime
            {
                PreviousPrevious, Previous, Later
            };

            static SulDownloader::suldownloaderdata::ProductReport upgradedBaseProduct()
            {
                SulDownloader::suldownloaderdata::ProductReport report;
                report.name = "BaseName";
                report.rigidName = "BaseRigidName";
                report.downloadedVersion = "0.5.0";
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


            static SulDownloader::suldownloaderdata::DownloadReport
            goodReport(UseTime useTime = UseTime::Later, bool upgraded = true, const std::string& sourceURL = "")
            {
                SulDownloader::suldownloaderdata::DownloadReport report;
                report.m_status = WarehouseStatus::SUCCESS;
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
                return report;
            }

            static std::string goodReportString(UseTime useTime)
            {
                auto report = goodReport(useTime);
                return SulDownloader::suldownloaderdata::DownloadReport::fromReport(report);
            }

            static SulDownloader::suldownloaderdata::DownloadReport
            badReport(UseTime useTime, WarehouseStatus status, std::string errorDescription)
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

                if (status != SulDownloader::WarehouseStatus::CONNECTIONERROR)
                {
                    report.m_productReport = goodProducts();
                    for (auto& product: report.m_productReport)
                    {
                        product.productStatus = SulDownloader::suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                    }

                }
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport getPluginFailedToInstallReport(UseTime useTime)
            {
                auto report = badReport(useTime, WarehouseStatus::INSTALLFAILED, "");
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
                for (auto& product: report->m_productReport)
                {
                    product.productStatus = suldownloaderdata::ProductReport::ProductStatus::UpToDate;
                }
            }


            static SulDownloader::suldownloaderdata::DownloadReport installFailedTwoProducts()
            {
                auto report = badReport(UseTime::Later, WarehouseStatus::INSTALLFAILED, "");
                report.m_productReport[0].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[1].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[0].errorDescription = "Base failed to install";
                report.m_productReport[1].errorDescription = "Plugin failed to install";
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport installCaughtError()
            {
                auto report = badReport(UseTime::Later, WarehouseStatus::UNSPECIFIED, "Error associated to install");
                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport downloadFailedError()
            {
                auto report = badReport(UseTime::Later, WarehouseStatus::DOWNLOADFAILED, "Error associated to install");
                report.m_productReport[0].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[1].productStatus = suldownloaderdata::ProductReport::ProductStatus::SyncFailed;
                report.m_productReport[0].errorDescription = "Base failed to download";
                report.m_productReport[1].errorDescription = "Plugin failed to download";

                return report;
            }


            static SulDownloader::suldownloaderdata::DownloadReport baseProductMissing()
            {
                auto report = badReport(UseTime::Later, WarehouseStatus::PACKAGESOURCEMISSING, "BaseName");

                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport bothProductsMissing()
            {
                auto report = badReport(UseTime::Later, WarehouseStatus::PACKAGESOURCEMISSING, "BaseName;PluginName");

                return report;
            }

            static SulDownloader::suldownloaderdata::DownloadReport connectionError()
            {
                auto report = badReport(UseTime::Later, WarehouseStatus::CONNECTIONERROR, "Failed to synchronize");
                return report;
            }
        };
    }
}
