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
                MustKeepReport, RedundantReport
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
            /**
             * Produces a ReportCollectionResult from the list of DownloadReports.
             * @pre It requires the DownloadReport to be given in chronological order with the most recent as the latest one.
             *
             * The returned SchedulerEvent.IsRelevantToSend does not consider if the latest event was sent 48 hours previously.
             * It is left to the caller to handle this.
             *
             * @param reportCollection
             * @return ReportCollectionResult
             */
            static ReportCollectionResult
            processReports(const std::vector<SulDownloader::DownloadReport>& reportCollection);


            /**
             * Indirect call to the processReports that use the directory of suldownloader reports to list the reports.
             *
             * It loads the SulDownloads from the directory path getSulDownloaderReportPath. Load the files into DownloadReport,
             * sort them in chronological order before passing them to the overloaded method that handles vector of DownloadReport.
             *
             * @return ReportAndFiles
             */
            static ReportAndFiles processReports();

        private:
            static ReportCollectionResult
            handleSuccessReports(const std::vector<SulDownloader::DownloadReport>& reportCollection);

            static ReportCollectionResult
            handleFailureReports(const std::vector<SulDownloader::DownloadReport>& reportCollection);

            static bool eventsAreDifferent(const UpdateEvent& lhs, const UpdateEvent& rhs);

            static bool hasUpgrade(const SulDownloader::DownloadReport& report);

            static int lastUpgrade(const std::vector<SulDownloader::DownloadReport>&);

            static int lastGoodSync(const std::vector<SulDownloader::DownloadReport>&);

        };

    }

}
