//
// Created by pair on 06/06/18.
//

#include "DownloadReport.h"
namespace SulDownloader
{

    DownloadReport::DownloadReport()
    {

    }

    DownloadReport DownloadReport::Report(const Warehouse &, const TimeTracker &timeTracker)
    {
        return DownloadReport();
    }

    DownloadReport DownloadReport::Report(const std::vector<Product> &, const TimeTracker &timeTracker)
    {
        return DownloadReport();
    }

    DownloadReport::Status DownloadReport::getStatus() const
    {
        return m_status;
    }

    std::string DownloadReport::getDescription() const
    {
        return m_descrtipion;
    }

    std::time_t DownloadReport::startTime() const
    {
        return m_startTime;
    }

    std::time_t DownloadReport::finishedTime() const
    {
        return m_finishedTime;
    }

    std::time_t DownloadReport::syncTime() const
    {
        return m_sync_time;
    }

    std::time_t DownloadReport::installTime() const
    {
        return m_installTime;
    }

    std::vector<ProductReport> DownloadReport::products() const
    {
        return std::vector<ProductReport>();
    }


}