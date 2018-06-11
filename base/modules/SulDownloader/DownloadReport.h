//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_DOWNLOADREPORT_H
#define EVEREST_DOWNLOADREPORT_H


#include <vector>
#include <ctime>
#include <string>
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
        enum class Status{ SUCCESS, INSTALLFAILLED, DOWNLOADFAILED, RESTARTNEEDED, CONNECTIONERROR, PACKAGESOURCEMISSING };
        static DownloadReport Report( const Warehouse & , const TimeTracker & timeTracker);
        static DownloadReport Report(const std::vector<Product> &, const TimeTracker &  timeTracker);

        Status getStatus() const;
        std::string getDescription() const;
        std::time_t startTime() const;
        std::time_t finishedTime() const;
        std::time_t syncTime() const;
        std::time_t installTime() const;
        std::vector<ProductReport> products() const;

    private:
        Status m_status;
        std::string m_descrtipion;
        std::time_t m_startTime;
        std::time_t m_finishedTime;
        std::time_t m_sync_time;
        std::time_t m_installTime;
    };

}


#endif //EVEREST_DOWNLOADREPORT_H
