/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <SulDownloader/DownloadReport.h>
#include "UpdateEvent.h"
#include "UpdateStatus.h"
namespace UpdateScheduler
{

    struct ReportCollectionResult
    {
        UpdateEvent SchedulerEvent;
        UpdateStatus SchedulerStatus;
        enum class SignificantReportMark {MustKeepReport, RedundantReport};
        std::vector<SignificantReportMark> IndicesOfSignificantReports;
    };

    struct ReportAndFiles
    {
        ReportCollectionResult reportCollectionResult;
        std::vector<std::string> filePaths;
    };

    class DownloadReportsAnalyser
    {
    public:
        static ReportCollectionResult processReports( const std::vector<SulDownloader::DownloadReport> & reportCollection);

        static ReportCollectionResult processReports( const std::vector<std::string> & filePaths );

        static ReportAndFiles processReports();

    private:
        static ReportCollectionResult handleSuccessReports(const std::vector<SulDownloader::DownloadReport> & reportCollection);
        static ReportCollectionResult handleFailureReports(const std::vector<SulDownloader::DownloadReport> & reportCollection);
        static bool eventsAreDifferent( const UpdateEvent & lhs, const UpdateEvent & rhs);
        static bool hasUpgrade( const SulDownloader::DownloadReport & report);
        static int lastUpgrade( const std::vector<SulDownloader::DownloadReport> &);
        static int lastGoodSync(const std::vector<SulDownloader::DownloadReport> &);

    };

}



