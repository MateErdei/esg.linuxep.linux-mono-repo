/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "UpdateEvent.h"
#include "UpdateStatus.h"

#include <SulDownloader/suldownloaderdata/DownloadReport.h>

#include <string>
#include <vector>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        struct ReportCollectionResult
        {
            UpdateEvent SchedulerEvent;
            UpdateStatus SchedulerStatus;
            enum class SignificantReportMark
            {
                MustKeepReport,
                RedundantReport
            };
            std::vector<SignificantReportMark> IndicesOfSignificantReports;
        };

        struct ReportAndFiles
        {
            ReportCollectionResult reportCollectionResult;
            std::vector<std::string> sortedFilePaths;
        };

        class DownloadReportsAnalyser
        {
        public:
            using DownloadReport = SulDownloader::suldownloaderdata::DownloadReport;
            using DownloadReportVector = std::vector<DownloadReport>;
            using DownloadReportVectorSizeType = DownloadReportVector::size_type;
            using DownloadReportVectorDifferenceType = DownloadReportVector::difference_type;
            /**
             * Produces a ReportCollectionResult from the list of DownloadReports.
             * @pre It requires the DownloadReport to be given in chronological order with the most recent as the latest
             * one.
             *
             * The returned SchedulerEvent.IsRelevantToSend does not consider if the latest event was sent 48 hours
             * previously. It is left to the caller to handle this.
             *
             * @param reportCollection
             * @return ReportCollectionResult
             */
            static ReportCollectionResult processReports(const DownloadReportVector& reportCollection);

            /**
             * Indirect call to the processReports that use the directory of suldownloader reports to list the reports.
             *
             * It loads the SulDownloads from the directory path getSulDownloaderReportPath. Load the files into
             * DownloadReport, sort them in chronological order before passing them to the overloaded method that
             * handles vector of DownloadReport.
             *
             * @return ReportAndFiles
             */
            static ReportAndFiles processReports();

            /**
             * Get the time of the last product update check, by looking at saved reports
             * @return
             */
            static time_t getLastProductUpdateCheck();

        private:

            struct FileAndDownloadReport
            {
                std::string filepath;
                SulDownloader::suldownloaderdata::DownloadReport report;
                std::string sortKey;
            };
            using FileAndDownloadReportVector = std::vector<FileAndDownloadReport>;
            static FileAndDownloadReportVector readSortedReports();

            static ReportCollectionResult handleSuccessReports(const DownloadReportVector& reportCollection);

            static ReportCollectionResult handleFailureReports(const DownloadReportVector& reportCollection);

            static bool eventsAreDifferent(const UpdateEvent& lhs, const UpdateEvent& rhs);

            static bool hasUpgrade(const DownloadReport& report);

            static DownloadReportVectorDifferenceType lastUpgrade(const DownloadReportVector&);

            static DownloadReportVectorDifferenceType lastGoodSync(const DownloadReportVector&);
            static time_t convertStringTimeToTimet(const std::string& basicString);
        };

    } // namespace configModule

} // namespace UpdateSchedulerImpl
