/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadReport.h"
#include "IWarehouseRepository.h"
#include "DownloadedProduct.h"
#include "TimeTracker.h"
#include "MessageUtility.h"


namespace SulDownloader
{

    DownloadReport::DownloadReport()
    {

    }

    DownloadReport DownloadReport::Report(const IWarehouseRepository & warehouse, const TimeTracker &timeTracker)
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
        report.m_status = report.setProductsInfo(warehouse.getProducts(), report.m_status);
        return report;
    }

    DownloadReport DownloadReport::Report(const std::vector<DownloadedProduct> & products, const TimeTracker &timeTracker)
    {
        DownloadReport report;
        report.setTimings(timeTracker);
        report.m_status = WarehouseStatus::SUCCESS;
        report.m_description = "";

        if(products.size() == 0)
        {
            // empty list means no products should have been downloaded.
            report.m_status = WarehouseStatus::DOWNLOADFAILED;
        }

        for( const auto & product: products)
        {
            if ( product.hasError())
            {
                report.m_description = "Update failed";
                report.m_status = WarehouseStatus::INSTALLFAILED;
            }
        }

        report.m_status = report.setProductsInfo(products, report.m_status);

        return report;
    }

    DownloadReport DownloadReport::Report(const std::string &errorDescription)
    {
        DownloadReport report;
        TimeTracker tt;
        tt.setStartTime(TimeTracker::getCurrTime());
        report.setTimings(tt);
        report.m_description = errorDescription;
        report.m_status = WarehouseStatus::UNSPECIFIED;
        return report;
    }



    WarehouseStatus DownloadReport::getStatus() const
    {
        return m_status;
    }

    const std::string& DownloadReport::getDescription() const
    {
        return m_description;
    }

    const std::string &DownloadReport::getStartTime() const
    {
        return m_startTime;
    }

    const std::string &DownloadReport::getFinishedTime() const
    {
        return m_finishedTime;
    }

    const std::string &DownloadReport::getSyncTime() const
    {
        return m_sync_time;
    }


    const std::vector<ProductReport>& DownloadReport::getProducts() const
    {
        return m_productReport;
    }

    WarehouseStatus DownloadReport::setProductsInfo(const std::vector<DownloadedProduct> &products, WarehouseStatus status)
    {

        m_productReport.clear();
        for (auto product : products)
        {
            ProductReport report;
            auto info = product.getProductMetadata();
            report.rigidName = info.getLine();
            report.name = info.getName();
            report.downloadedVersion = info.getVersion();
            report.installedVersion = product.getPostUpdateInstalledVersion();
            auto wError = product.getError();
            report.errorDescription = wError.Description;
            m_productReport.push_back(report);

            // ensure that an error status is reported
            if(report.downloadedVersion != report.installedVersion && status == WarehouseStatus::SUCCESS)
            {
                status = WarehouseStatus::INSTALLFAILED;
            }
        }

        return status;

    }

    void DownloadReport::setTimings(const TimeTracker &timeTracker)
    {
        m_startTime = timeTracker.startTime();
        m_sync_time = timeTracker.syncTime();
        m_finishedTime = timeTracker.finishedTime();
    }

    const std::string& DownloadReport::getSulError() const
    {
        return m_sulError;
    }

    void DownloadReport::setError(const WarehouseError &error)
    {
        m_description = error.Description;
        m_status = error.status;
        m_sulError = error.SulError;
    }

    int DownloadReport::getExitCode() const
    {
        return static_cast<int>( m_status);
    }


    // add one return the json content directly.
    SulDownloaderProto::DownloadStatusReport DownloadReport::fromReport( const DownloadReport & report)
    {
        SulDownloaderProto::DownloadStatusReport protoReport;
        protoReport.set_starttime(report.getStartTime());
        protoReport.set_finishtime(report.getFinishedTime());
        protoReport.set_synctime(report.getSyncTime());

        protoReport.set_status( toString( report.getStatus()));
        protoReport.set_errordescription(report.getDescription());
        protoReport.set_sulerror(report.getSulError());

        for ( auto & product : report.getProducts())
        {
            SulDownloaderProto::ProductStatusReport * productReport = protoReport.add_products();
            productReport->set_productname( product.name);
            productReport->set_rigidname( product.rigidName);
            productReport->set_downloadversion( product.downloadedVersion);
            productReport->set_installedversion( product.installedVersion);
            productReport->set_errordescription( product.errorDescription);
        }

        return protoReport;
    }

    std::tuple<int, std::string> DownloadReport::CodeAndSerialize(const DownloadReport &report)
    {
            auto protoReport = DownloadReport::fromReport(report);
            std::string json = MessageUtility::protoBuf2Json(protoReport);
            return std::tuple<int, std::string>(report.getExitCode() , json );
    }



}