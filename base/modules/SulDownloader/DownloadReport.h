/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once



#include <vector>
#include <ctime>
#include <string>
#include "WarehouseError.h"


#include "DownloadReport.pb.h"

namespace SulDownloader
{
    class IWarehouseRepository;
    class DownloadedProduct;
    class TimeTracker;

    struct ProductReport
    {
        std::string name;
        std::string rigidName;
        std::string downloadedVersion;
        std::string installedVersion;
        std::string errorDescription;
        bool upgraded;
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
        static DownloadReport Report( const IWarehouseRepository & , const TimeTracker & timeTracker);
        static DownloadReport Report(const std::vector<DownloadedProduct> &, const TimeTracker &  timeTracker);
        static DownloadReport Report(const std::string & errorDescription);
        static std::tuple<int, std::string> CodeAndSerialize(const DownloadReport &report);
        static SulDownloaderProto::DownloadStatusReport fromReport( const DownloadReport & report);

        WarehouseStatus getStatus() const;
        const std::string& getDescription() const;
        const std::string& getSulError() const;
        const std::string &getStartTime() const;
        const std::string &getFinishedTime() const;
        const std::string &getSyncTime() const;
        const std::vector<ProductReport>& getProducts() const;

        int getExitCode() const;

    private:
        WarehouseStatus  m_status;
        std::string m_description;
        std::string m_sulError;
        std::string m_startTime;
        std::string m_finishedTime;
        std::string m_sync_time;

        std::vector<ProductReport> m_productReport;

        WarehouseStatus setProductsInfo(const std::vector<DownloadedProduct> &products, WarehouseStatus status);
        void setError( const WarehouseError & error);
        void setTimings( const TimeTracker & );
    };

}



