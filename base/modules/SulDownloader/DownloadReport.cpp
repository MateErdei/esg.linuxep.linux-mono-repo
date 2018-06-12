//
// Created by pair on 06/06/18.
//

#include "DownloadReport.h"
#include "Warehouse.h"
#include "Product.h"
#include "TimeTracker.h"
namespace SulDownloader
{

    DownloadReport::DownloadReport()
    {

    }

    DownloadReport DownloadReport::Report(const Warehouse & warehouse, const TimeTracker &timeTracker)
    {
        DownloadReport report;
        report.setTimings(timeTracker);
        if ( warehouse.hasError())
        {
            report.setError(warehouse.getError());
        }
        else
        {
            report.m_status = WarehouseStatus::SUCCESS;
            report.m_description = "";
        }
        report.setProductsInfo(warehouse.getProducts());
        return report;
    }

    DownloadReport DownloadReport::Report(const std::vector<Product> & products, const TimeTracker &timeTracker)
    {
        DownloadReport report;
        report.setTimings(timeTracker);
        report.m_status = WarehouseStatus::SUCCESS;
        report.m_description = "";
        for( const auto & product: products)
        {
            if ( product.hasError())
            {
                report.setError(product.getError());
            }
        }
        report.setProductsInfo(products);
        return report;
    }

    WarehouseStatus DownloadReport::getStatus() const
    {
        return m_status;
    }

    std::string DownloadReport::getDescription() const
    {
        return m_description;
    }

    const std::string &DownloadReport::startTime() const
    {
        return m_startTime;
    }

    const std::string &DownloadReport::finishedTime() const
    {
        return m_finishedTime;
    }

    const std::string &DownloadReport::syncTime() const
    {
        return m_sync_time;
    }


    std::vector<ProductReport> DownloadReport::products() const
    {
        return m_productReport;
    }

    void DownloadReport::setProductsInfo(const std::vector<Product> &products)
    {

        m_productReport.clear();
        for (auto product : products)
        {
            ProductReport report;
            auto info = product.getProductInformation();
            report.rigidName = info.getLine();
            report.name = info.getName(); // TODO need to store actual product name.
            report.downloadedVersion = info.getBaseVersion(); // TODO should be actual version
            report.installedVersion = info.getBaseVersion(); // TODO should be actual installed version
            m_productReport.push_back(report);
        }

    }

    void DownloadReport::setTimings(const TimeTracker &timeTracker)
    {
        m_startTime = timeTracker.startTime();
        m_sync_time = timeTracker.syncTime();
        m_finishedTime = timeTracker.finishedTime();
    }

    std::string DownloadReport::sulError() const
    {
        return m_sulError;
    }

    void DownloadReport::setError(const WarehouseError &error)
    {
        m_description = error.Description;
        m_status = error.status;
        m_sulError = error.SulError;
    }

    int DownloadReport::exitCode()
    {
        return static_cast<int>( m_status);
    }


}