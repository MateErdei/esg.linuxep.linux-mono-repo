/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadReport.h"
#include "IWarehouseRepository.h"
#include "DownloadedProduct.h"
#include "TimeTracker.h"
#include <Common/UtilityImpl/MessageUtility.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include "Logger.h"
#include "SulDownloaderException.h"
#include <DownloadReport.pb.h>

#include <sstream>
#include <google/protobuf/util/json_util.h>


namespace SulDownloader
{
    using namespace Common::UtilityImpl;
    DownloadReport::DownloadReport(): m_status(WarehouseStatus::UNSPECIFIED)
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

    DownloadReport DownloadReport::Report(const std::vector<DownloadedProduct> & products, TimeTracker *timeTracker, VerifyState verifyState)
    {
        assert(timeTracker != nullptr);
        DownloadReport report;

        report.m_status = WarehouseStatus::SUCCESS;
        report.m_description = "";

        if(products.empty())
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

        if ( verifyState == VerifyState::VerifyFailed)
        {
            for ( auto & productReport : report.m_productReport)
            {
                productReport.productStatus = ProductReport::ProductStatus::SyncFailed;
            }
        }


        if (report.m_status == WarehouseStatus::SUCCESS)
        {
            timeTracker->setSyncTime();
        }

        report.setTimings(*timeTracker);
        return report;
    }

    DownloadReport DownloadReport::Report(const std::string &errorDescription)
    {
        DownloadReport report;
        TimeTracker tt;
        tt.setStartTime(TimeUtils::getCurrTime());
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
        for (auto & product : products)
        {
            ProductReport productReportEntry;
            auto & info = product.getProductMetadata();
            productReportEntry.rigidName = info.getLine();
            productReportEntry.name = info.getName();
            productReportEntry.downloadedVersion = info.getVersion();
            productReportEntry.installedVersion = product.getPostUpdateInstalledVersion();
            productReportEntry.productStatus = product.productHasChanged()? ProductReport::ProductStatus::Upgraded : ProductReport::ProductStatus::UpToDate;
            auto wError = product.getError();
            productReportEntry.errorDescription = wError.Description;


            if ( !productReportEntry.errorDescription.empty())
            {
                productReportEntry.productStatus = ProductReport::ProductStatus::SyncFailed;
            }

            // ensure that an error status is reported
            if (productReportEntry.downloadedVersion != productReportEntry.installedVersion &&
                status == WarehouseStatus::SUCCESS)
            {
                std::stringstream errorInfo;
                errorInfo << "Downloaded version: " << productReportEntry.downloadedVersion
                          << " differs from the installed version: "
                          << productReportEntry.installedVersion;
                LOGERROR(errorInfo.str());
                status = WarehouseStatus::INSTALLFAILED;
                if (productReportEntry.errorDescription.empty())
                {
                    productReportEntry.errorDescription = errorInfo.str();
                }

            }

            m_productReport.push_back(productReportEntry);
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
    std::string DownloadReport::fromReport(const DownloadReport &report)
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
            productReport->set_upgraded(product.productStatus == ProductReport::ProductStatus::Upgraded);
        }

        return Common::UtilityImpl::MessageUtility::protoBuf2Json(protoReport);
    }

    DownloadReport DownloadReport::toReport(const std::string &serializedVersion)
    {
        using namespace google::protobuf::util;
        using SulDownloaderProto::DownloadStatusReport;

        DownloadStatusReport protoReport;
        auto status = JsonStringToMessage(serializedVersion, &protoReport );
        if ( !status.ok())
        {
            LOGERROR("Failed to load SulDownload serialized string");
            LOGSUPPORT(status.ToString());
            throw SulDownloaderException( "Failed to load SulDownloadReport from string");
        }

        DownloadReport report;
        report.m_startTime = protoReport.starttime();
        report.m_finishedTime = protoReport.finishtime();
        report.m_sync_time = protoReport.synctime();

        fromString(protoReport.status(), &report.m_status);

        report.m_description = protoReport.errordescription();
        report.m_sulError = protoReport.sulerror();

        for( auto & protoProduct : protoReport.products())
        {
            ProductReport productReport;
            productReport.rigidName = protoProduct.rigidname();
            productReport.name = protoProduct.productname();
            productReport.downloadedVersion = protoProduct.downloadversion();
            productReport.installedVersion = protoProduct.installedversion();
            productReport.errorDescription = protoProduct.errordescription();
            if ( protoProduct.upgraded())
            {
                productReport.productStatus = ProductReport::ProductStatus::Upgraded;
            }
            else if ( productReport.errorDescription.empty())
            {
                productReport.productStatus = ProductReport::ProductStatus::UpToDate;
            }
            else
            {
                productReport.productStatus = ProductReport::ProductStatus::SyncFailed;
            }

            report.m_productReport.push_back(productReport);
        }
        return report;
    }


    std::tuple<int, std::string> DownloadReport::CodeAndSerialize(const DownloadReport &report)
    {

            std::string json = DownloadReport::fromReport(report);
            return std::tuple<int, std::string>(report.getExitCode() , json );
    }

    const std::string DownloadReport::getSourceURL() const
    {
        //FIXME: implement the get Source
        return "Sophos";
    }



    std::string ProductReport::statusToString() const
    {
        switch (productStatus)
        {
            case ProductStatus::Upgraded:
                return "Upgraded";
            case ProductStatus::SyncFailed:
                return "SyncFailed";
            case ProductStatus::UpToDate:
            default:
                return "UpToDate";
        }
    }
}