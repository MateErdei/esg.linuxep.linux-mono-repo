/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef EVEREST_DOWNLOADREPORT_H
#define EVEREST_DOWNLOADREPORT_H


#include <vector>
#include <ctime>
#include <string>
#include "WarehouseError.h"


#include "DownloadReport.pb.h"

namespace SulDownloader
{
    class Warehouse;
    class Product;
    class TimeTracker;

    struct ProductReport
    {
        std::string name;
        std::string rigidName;
        std::string downloadedVersion;
        std::string installedVersion;
    };

    class DownloadReport
    {
        DownloadReport();
    public:
        static DownloadReport Report( const Warehouse & , const TimeTracker & timeTracker);
        static DownloadReport Report(const std::vector<Product> &, const TimeTracker &  timeTracker);
        static DownloadReport Report(const std::string & errorDescription);
        static std::tuple<int, std::string> CodeAndSerialize(const DownloadReport & report);
        static SulDownloaderProto::DownloadStatusReport fromReport( const DownloadReport & report);

        WarehouseStatus getStatus() const;
        std::string getDescription() const;
        std::string sulError() const;
        const std::string &startTime() const;
        const std::string &finishedTime() const;
        const std::string &syncTime() const;
        std::vector<ProductReport> products() const;

        int exitCode() const;

    private:
        WarehouseStatus  m_status;
        std::string m_description;
        std::string m_sulError;
        std::string m_startTime;
        std::string m_finishedTime;
        std::string m_sync_time;

        std::vector<ProductReport> m_productReport;

        void setProductsInfo(const std::vector<Product> &products);
        void setError( const WarehouseError & error);
        void setTimings( const TimeTracker & );
    };

}


#endif //EVEREST_DOWNLOADREPORT_H
