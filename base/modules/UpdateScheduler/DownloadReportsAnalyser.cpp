/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DownloadReportsAnalyser.h"
namespace UpdateScheduler
{

    ReportCollectionResult
    DownloadReportsAnalyser::processReports(const std::vector<SulDownloader::DownloadReport> &reportCollection)
    {
        return ReportCollectionResult();
    }

    ReportCollectionResult DownloadReportsAnalyser::processReports(const std::vector<std::string> &filePaths)
    {
        return ReportCollectionResult();
    }

    ReportAndFiles DownloadReportsAnalyser::processReports()
    {
        return ReportAndFiles();
    }
}