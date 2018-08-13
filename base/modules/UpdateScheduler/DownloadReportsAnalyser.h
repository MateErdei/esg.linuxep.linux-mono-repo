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

    };

}



