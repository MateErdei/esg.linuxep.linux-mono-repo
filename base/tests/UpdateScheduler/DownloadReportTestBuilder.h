// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once
#include "Common/DownloadReport/DownloadReport.h"
#include "Common/DownloadReport/ProductInfo.h"
#include "Common/DownloadReport/RepositoryStatus.h"

#include <string>

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

        static Common::DownloadReport::ProductReport upgradedBaseProduct()
        {
            Common::DownloadReport::ProductReport report;
            report.name = "BaseName";
            report.rigidName = "BaseRigidName";
            report.downloadedVersion = "0.5.0";
            report.installedVersion = "0.5.0";
            report.errorDescription.clear();
            report.productStatus = Common::DownloadReport::ProductReport::ProductStatus::Upgraded;
            return report;
        }

        static Common::DownloadReport::ProductReport upgradedPlugin()
        {
            Common::DownloadReport::ProductReport report;
            report.name = "PluginName";
            report.rigidName = "PluginRigidName";
            report.downloadedVersion = "0.5.0";
            report.installedVersion = "0.5.0";
            report.errorDescription.clear();
            report.productStatus = Common::DownloadReport::ProductReport::ProductStatus::Upgraded;
            return report;
        }

        static std::vector<Common::DownloadReport::ProductReport> goodProducts()
        {
            std::vector<Common::DownloadReport::ProductReport> products;
            products.push_back(upgradedBaseProduct());
            products.push_back(upgradedPlugin());
            return products;
        }

        static Common::DownloadReport::DownloadReport goodReport(
            UseTime useTime = UseTime::Later,
            bool upgraded = true,
            const std::string& sourceURL = "")
        {
            Common::DownloadReport::DownloadReport report;
            report.m_status = Common::DownloadReport::RepositoryStatus::SUCCESS;
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

            report.m_productReport = goodProducts();
            if (!upgraded)
            {
                for (auto& product : report.m_productReport)
                {
                    product.productStatus = Common::DownloadReport::ProductReport::ProductStatus::UpToDate;
                }
            }

            Common::DownloadReport::ProductReport baseMetadata = report.m_productReport[0];
            Common::DownloadReport::ProductInfo productInfo;
            productInfo.m_productName = baseMetadata.name;
            productInfo.m_version = baseMetadata.downloadedVersion;
            productInfo.m_installedVersion = baseMetadata.installedVersion;
            productInfo.m_rigidName = baseMetadata.rigidName;
            report.m_repositoryComponents.push_back(productInfo);

            return report;
        }

        static Common::DownloadReport::DownloadReport goodReportWithSubscriptions()
        {
            auto report = goodReport();
            std::vector<Common::DownloadReport::ProductReport> products;
            Common::DownloadReport::ProductReport compSuite;
            compSuite.rigidName = "ComponentSuite";
            compSuite.downloadedVersion = "v1";
            compSuite.installedVersion="0.5.0";
            products.push_back(compSuite);
            report.m_productReport = products;
            return report;
        }

        static Common::DownloadReport::DownloadReport goodReportWithBaseRigidNameSubscriptions()
        {
            auto report = goodReport();
            std::vector<Common::DownloadReport::ProductReport> products;
            Common::DownloadReport::ProductReport compSuite;
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
            return Common::DownloadReport::DownloadReport::fromReport(report);
        }

        static Common::DownloadReport::DownloadReport badReport(
            UseTime useTime,
            Common::DownloadReport::RepositoryStatus status,
            std::string errorDescription)
        {
            Common::DownloadReport::DownloadReport report;
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

            if (status != Common::DownloadReport::RepositoryStatus::CONNECTIONERROR)
            {
                report.m_productReport = goodProducts();
                for (auto& product : report.m_productReport)
                {
                    product.productStatus =
                        Common::DownloadReport::ProductReport::ProductStatus::SyncFailed;
                }
            }
            return report;
        }

        static Common::DownloadReport::DownloadReport getPluginFailedToInstallReport(UseTime useTime)
        {
            auto report = badReport(useTime, Common::DownloadReport::RepositoryStatus::INSTALLFAILED, "");
            report.m_productReport[0].productStatus = Common::DownloadReport::ProductReport::ProductStatus::Upgraded;
            report.m_productReport[1].productStatus = Common::DownloadReport::ProductReport::ProductStatus::SyncFailed;
            report.m_productReport[1].errorDescription = "Plugin failed to install";
            return report;
        }

        static std::string getPluginFailedToInstallReportString(UseTime useTime)
        {
            Common::DownloadReport::DownloadReport report = getPluginFailedToInstallReport(useTime);
            return Common::DownloadReport::DownloadReport::fromReport(report);
        }

        static void setReportNoUpgrade(Common::DownloadReport::DownloadReport* report)
        {
            for (auto& product : report->m_productReport)
            {
                product.productStatus = Common::DownloadReport::ProductReport::ProductStatus::UpToDate;
            }
        }

        static Common::DownloadReport::DownloadReport installFailedTwoProducts()
        {
            auto report = badReport(UseTime::Later, Common::DownloadReport::RepositoryStatus::INSTALLFAILED, "");
            report.m_productReport[0].productStatus = Common::DownloadReport::ProductReport::ProductStatus::SyncFailed;
            report.m_productReport[1].productStatus = Common::DownloadReport::ProductReport::ProductStatus::SyncFailed;
            report.m_productReport[0].errorDescription = "Base failed to install";
            report.m_productReport[1].errorDescription = "Plugin failed to install";
            return report;
        }

        static Common::DownloadReport::DownloadReport installCaughtError()
        {
            auto report = badReport(UseTime::Later, Common::DownloadReport::RepositoryStatus::UNSPECIFIED, "Error associated to install");
            return report;
        }

        static Common::DownloadReport::DownloadReport downloadFailedError()
        {
            auto report = badReport(UseTime::Later, Common::DownloadReport::RepositoryStatus::DOWNLOADFAILED, "https://my.update.site/");
            // When we get a download failure, we can't tell which product had the failure.
            // To match this the unit test also doesn't put errorDescription in the product reports.
            report.m_productReport[0].productStatus = Common::DownloadReport::ProductReport::ProductStatus::UpToDate;
            report.m_productReport[1].productStatus = Common::DownloadReport::ProductReport::ProductStatus::UpToDate;
            report.m_productReport[0].errorDescription = "";
            report.m_productReport[1].errorDescription = "";

            return report;
        }

        static Common::DownloadReport::DownloadReport baseProductMissing()
        {
            auto report = badReport(UseTime::Later, Common::DownloadReport::RepositoryStatus::PACKAGESOURCEMISSING, "BaseName");

            return report;
        }

        static Common::DownloadReport::DownloadReport bothProductsMissing()
        {
            auto report = badReport(UseTime::Later, Common::DownloadReport::RepositoryStatus::PACKAGESOURCEMISSING, "BaseName;PluginName");

            return report;
        }

        static Common::DownloadReport::DownloadReport connectionError()
        {
            auto report = badReport(UseTime::Later, Common::DownloadReport::RepositoryStatus::CONNECTIONERROR, "Failed to synchronize");
            return report;
        }
    };
