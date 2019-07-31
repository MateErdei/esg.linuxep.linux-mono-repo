/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IWarehouseRepository.h"
#include "WarehouseError.h"

#include <ctime>
#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class DownloadedProduct;
        class TimeTracker;

        struct ProductReport
        {
            std::string name;
            std::string rigidName;
            std::string downloadedVersion;
            std::string errorDescription;
            enum class ProductStatus
            {
                SyncFailed,
                InstallFailed,
                UninstallFailed,
                VerifyFailed,
                UpToDate,
                Upgraded,
                Uninstalled
            };
            ProductStatus productStatus = ProductStatus::SyncFailed;

            std::string statusToString() const;

            std::string jsonString() const;
        };

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
            DownloadReport();

        public:
            friend class DownloadReportTestBuilder;

            enum class VerifyState
            {
                VerifyFailed,
                VerifyCorrect
            };

            static DownloadReport Report(const IWarehouseRepository&, const TimeTracker& timeTracker);

            static DownloadReport Report(
                const std::string& sourceURL,
                const std::vector<suldownloaderdata::DownloadedProduct>& products,
                const std::vector<suldownloaderdata::ProductInfo>& componentsToALCStatus,
                TimeTracker* timeTracker,
                VerifyState verify);

            static DownloadReport Report(const std::string& errorDescription);

            static std::vector<std::string> listOfAllPreviousReports(const std::string& outputParentPath);

            static std::tuple<int, std::string> CodeAndSerialize(const DownloadReport& report);

            static std::string fromReport(const DownloadReport& report);

            static DownloadReport toReport(const std::string& serializedVersion);

            WarehouseStatus getStatus() const;

            const std::string& getDescription() const;

            const std::string& getSulError() const;

            const std::string& getStartTime() const;

            const std::string& getFinishedTime() const;

            const std::string& getSyncTime() const;

            const std::vector<ProductReport>& getProducts() const;

            const std::vector<ProductInfo>& getWarehouseComponents() const;

            const std::string getSourceURL() const;

            int getExitCode() const;

        private:
            WarehouseStatus m_status;
            std::string m_description;
            std::string m_sulError;
            std::string m_startTime;
            std::string m_finishedTime;
            std::string m_sync_time;
            std::string m_urlSource;

            std::vector<ProductReport> m_productReport;
            std::vector<ProductInfo> m_warehouseComponents;

            void setProductsInfo(
                const std::vector<suldownloaderdata::DownloadedProduct>& products,
                const WarehouseStatus& warehouseStatus);

            void setError(const WarehouseError& error);

            void setTimings(const TimeTracker&);
        };
    } // namespace suldownloaderdata
} // namespace SulDownloader
