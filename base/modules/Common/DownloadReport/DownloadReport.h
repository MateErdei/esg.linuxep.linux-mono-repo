// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ProductInfo.h"
#include "RepositoryStatus.h"

#include <ctime>
#include <string>
#include <string_view>
#include <vector>

class DownloadReportTestBuilder;
namespace SulDownloader::suldownloaderdata
{
    class DownloadReportBuilder;
}

namespace Common::DownloadReport
{
    struct ProductReport
    {
        std::string name;
        std::string rigidName;
        std::string downloadedVersion;
        std::string errorDescription;
        std::string installedVersion;
        enum class ProductStatus : int
        {
            UpToDate = 0,
            Upgraded = 1,
            Uninstalled = 2,
            UninstallFailed = 3,
            SyncFailed = 4,
            InstallFailed = 5,
            VerifyFailed = 6
        };
        ProductStatus productStatus = ProductStatus::SyncFailed;

        std::string statusToString() const;

        std::string jsonString() const;

        [[nodiscard]] bool operator==(const ProductReport& other) const
        {
            // clang-format off
            return name == other.name &&
                   rigidName == other.rigidName &&
                   downloadedVersion == other.downloadedVersion &&
                   errorDescription == other.errorDescription &&
                   installedVersion == other.installedVersion &&
                   productStatus == other.productStatus;
            // clang-format on
        }
    };
    bool operator<(ProductReport::ProductStatus lh, ProductReport::ProductStatus rh);
    /**
     * DownloadReport holds the relevant information about an attempt to run SULDownloader.
     * It will eventually be serialized to json via the SulDownloaderProto::DownloadStatusReport.
     *
     * Its intention is to provide the relevant information necessary to report Central as specified in the
     * following urls:
     * https://wiki.sophos.net/display/SophosCloud/EMP%3A+status-alc
     * https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
     *
     * Hence, it contains information about the installed entries as well as failures reasons.
     */
    class DownloadReport
    {
    public:
        friend class ::DownloadReportTestBuilder;
        friend class SulDownloader::suldownloaderdata::DownloadReportBuilder;

        static std::tuple<int, std::string> CodeAndSerialize(const DownloadReport& report);

        static std::string fromReport(const DownloadReport& report);

        static DownloadReport toReport(const std::string& serializedVersion);

        RepositoryStatus getStatus() const;
        void setStatus(const RepositoryStatus& status) { m_status = std::move(status); }

        const std::string& getDescription() const;

        const std::string& getSulError() const;

        const std::string& getStartTime() const;

        const std::string& getFinishedTime() const;

        const std::vector<ProductReport>& getProducts() const;

        const std::vector<ProductInfo>& getRepositoryComponents() const;

        const std::string getSourceURL() const;

        int getExitCode() const;

        bool isProcessedReport() const;

        void setProcessedReport(bool isProcessed);

        void combinePreviousReportIfRequired(const DownloadReport& previousReport);

        [[nodiscard]] bool isSupplementOnlyUpdate() const { return m_supplementOnly; }

        [[nodiscard]] bool isSuccesfulProductUpdateCheck() const;

        [[nodiscard]] bool wasBaseDowngraded() const { return m_baseDowngrade; }

        [[nodiscard]] bool operator==(const DownloadReport& other) const
        {
            // clang-format off
            return m_status == other.m_status &&
                   m_description == other.m_description &&
                   m_sulError == other.m_sulError &&
                   m_startTime == other.m_startTime &&
                   m_finishedTime == other.m_finishedTime &&
                   m_urlSource == other.m_urlSource &&
                   m_productReport == other.m_productReport &&
                   m_repositoryComponents == other.m_repositoryComponents &&
                   m_processedReport == other.m_processedReport &&
                   m_supplementOnly == other.m_supplementOnly &&
                   m_baseDowngrade == other.m_baseDowngrade;
            // clang-format on
        }

    private:
        void setProducts(const std::vector<ProductReport>& products);
        void setRepositoryComponents(const std::vector<ProductInfo>& repositoryComponents);

        RepositoryStatus m_status = RepositoryStatus::UNSPECIFIED;
        std::string m_description;
        std::string m_sulError;
        std::string m_startTime;
        std::string m_finishedTime;
        std::string m_urlSource;

        std::vector<ProductReport> m_productReport;
        std::vector<ProductInfo> m_repositoryComponents;

        bool m_processedReport = false;
        bool m_supplementOnly = false;
        bool m_baseDowngrade = false;
    };
} // namespace Common::DownloadReport
