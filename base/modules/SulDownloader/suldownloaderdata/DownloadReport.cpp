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

using namespace SulDownloader::suldownloaderdata;

namespace
{
    SulDownloaderProto::ProductStatusReport_ProductStatus convert(ProductReport::ProductStatus  productStatus)
    {
        using ProductStatus = ProductReport::ProductStatus;
        using namespace SulDownloaderProto;
        switch(productStatus)
        {
            case ProductStatus::UpToDate:
                return ProductStatusReport_ProductStatus_UPTODATE;
            case ProductStatus::Upgraded:
                return ProductStatusReport_ProductStatus_UPGRADED;
            case ProductStatus::Uninstalled:
                return ProductStatusReport_ProductStatus_UNINSTALLED;
            case ProductStatus::VerifyFailed:
                return ProductStatusReport_ProductStatus_VERIFYFAILED;
            case ProductStatus::InstallFailed:
                return ProductStatusReport_ProductStatus_INSTALLFAILED;
            case ProductStatus::UninstallFailed:
                return ProductStatusReport_ProductStatus_UNINSTALLFAILED;
            case ProductStatus::SyncFailed:
            default:
                return ProductStatusReport_ProductStatus_SYNCFAILED;
        }
    }

    ProductReport::ProductStatus convert(SulDownloaderProto::ProductStatusReport_ProductStatus protoProductStatus)
    {
        using ProductStatus = ProductReport::ProductStatus;
        using namespace SulDownloaderProto;

        switch (protoProductStatus)
        {

            case ProductStatusReport_ProductStatus_UPTODATE:
                return ProductStatus::UpToDate;
            case ProductStatusReport_ProductStatus_UPGRADED:
                return ProductStatus::Upgraded;
            case ProductStatusReport_ProductStatus_UNINSTALLED:
                return ProductStatus::Uninstalled;
            case ProductStatusReport_ProductStatus_VERIFYFAILED:
                return ProductStatus::VerifyFailed;
            case ProductStatusReport_ProductStatus_INSTALLFAILED:
                return ProductStatus::InstallFailed;
            case ProductStatusReport_ProductStatus_UNINSTALLFAILED:
                return ProductStatus::UninstallFailed;
            case ProductStatusReport_ProductStatus_SYNCFAILED:
            default:
                return ProductStatus::SyncFailed;

        }
    }

}
namespace SulDownloader
{
    using namespace Common::UtilityImpl;
    DownloadReport::DownloadReport(): m_status(WarehouseStatus::UNSPECIFIED)
    {

    }

    DownloadReport DownloadReport::Report(const suldownloaderdata::IWarehouseRepository & warehouse, const suldownloaderdata::TimeTracker &timeTracker)
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
        report.m_urlSource = warehouse.getSourceURL();
        report.setProductsInfo(warehouse.getProducts(), report.m_status);
        return report;
    }

    DownloadReport DownloadReport::Report(const std::string & sourceURL, const std::vector<suldownloaderdata::DownloadedProduct> & products, TimeTracker *timeTracker, VerifyState verifyState)
    {
        assert(timeTracker != nullptr);
        DownloadReport report;

        report.m_status = WarehouseStatus::SUCCESS;
        report.m_description = "";
        report.m_urlSource = sourceURL;

        if(products.empty())
        {
            // empty list means no products should have been downloaded.
            report.m_status = WarehouseStatus::DOWNLOADFAILED;
        }

        for( const auto & product: products)
        {
            if ( product.hasError() )
            {
                if (!product.getProductIsBeingUninstalled())
                {
                    report.m_description = "Update failed";
                    report.m_status = WarehouseStatus::INSTALLFAILED;
                    break; // Install failures are a higher error than uninstalled.
                }
                else
                {
                    report.m_description = "Uninstall failed";
                    report.m_status = WarehouseStatus::UNINSTALLFAILED;
                }
            }
        }

        report.setProductsInfo(products, report.m_status);

        if ( verifyState == VerifyState::VerifyFailed)
        {
            for ( auto & productReport : report.m_productReport)
            {
                productReport.productStatus = ProductReport::ProductStatus::VerifyFailed;
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

    void DownloadReport::setProductsInfo(const std::vector<suldownloaderdata::DownloadedProduct>& products,
                                         const WarehouseStatus& warehouseStatus)
    {
        m_productReport.clear();
        for (auto& product : products)
        {
            ProductReport productReportEntry;
            auto& info = product.getProductMetadata();
            productReportEntry.rigidName = info.getLine();
            productReportEntry.name = info.getName();
            productReportEntry.downloadedVersion = info.getVersion();
            auto wError = product.getError();
            productReportEntry.errorDescription = wError.Description;

            //Set product status appropriately
            if ((warehouseStatus == WarehouseStatus::DOWNLOADFAILED) ||
                (warehouseStatus == WarehouseStatus::PACKAGESOURCEMISSING))
            {
                productReportEntry.productStatus = ProductReport::ProductStatus::SyncFailed;
            }
            else
            {
                productReportEntry.productStatus = (product.productHasChanged() || product.forceProductReinstall())
                                                   ? ProductReport::ProductStatus::Upgraded
                                                   : ProductReport::ProductStatus::UpToDate;
                if (!productReportEntry.errorDescription.empty())
                {
                    if (!product.getProductIsBeingUninstalled())
                    {
                        productReportEntry.productStatus = ProductReport::ProductStatus::InstallFailed;
                    }
                    else
                    {
                        productReportEntry.productStatus = ProductReport::ProductStatus::UninstallFailed;
                    }
                }
                else if (product.getProductIsBeingUninstalled())
                {
                    productReportEntry.productStatus = ProductReport::ProductStatus::Uninstalled;
                }
            }

            m_productReport.push_back(productReportEntry);
        }
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
        protoReport.set_urlsource(report.getSourceURL());

        for ( auto & product : report.getProducts())
        {
            SulDownloaderProto::ProductStatusReport * productReport = protoReport.add_products();
            productReport->set_productname( product.name);
            productReport->set_rigidname( product.rigidName);
            productReport->set_downloadversion( product.downloadedVersion);
            productReport->set_errordescription( product.errorDescription);

            productReport->set_productstatus( convert(product.productStatus));
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
        report.m_urlSource = protoReport.urlsource();

        for( auto & protoProduct : protoReport.products())
        {
            ProductReport productReport;
            productReport.rigidName = protoProduct.rigidname();
            productReport.name = protoProduct.productname();
            productReport.downloadedVersion = protoProduct.downloadversion();
            productReport.errorDescription = protoProduct.errordescription();
            productReport.productStatus = convert(protoProduct.productstatus());

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
        return m_urlSource;
    }



    std::string ProductReport::statusToString() const
    {
        switch (productStatus)
        {
            case ProductStatus::Upgraded:
                return "Upgraded";
            case ProductStatus::SyncFailed:
                return "SyncFailed";
            case ProductStatus::VerifyFailed:
                return "VerifyFailed";
            case ProductStatus::InstallFailed:
                return "InstallFailed";
            case ProductStatus::UninstallFailed:
                return "UninstallFailed";
            case ProductStatus::Uninstalled:
                return "Uninstalled";
            case ProductStatus::UpToDate:
            default:
                return "UpToDate";
        }
    }

    std::string ProductReport::jsonString() const
    {
        switch (productStatus)
        {
            case ProductStatus::Upgraded:
                return "UPGRADED";
            case ProductStatus::SyncFailed:
                return "SYNCFAILED";
            case ProductStatus::VerifyFailed:
                return "VERIFYFAILED";
            case ProductStatus::InstallFailed:
                return "INSTALLFAILED";
            case ProductStatus::UninstallFailed:
                return "UNINSTALLFAILED";
            case ProductStatus::Uninstalled:
                return "UNINSTALLED";
            case ProductStatus::UpToDate:
            default:
                return "UPTODATE";
        }
    }
}