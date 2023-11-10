// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "DownloadReport.h"

#include "DownloadReportParseException.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/DownloadReport/DownloadReport.pb.h"
#include "Common/ProtobufUtil/MessageUtility.h"

#include <google/protobuf/util/json_util.h>

#include <sstream>

using namespace Common::DownloadReport;

namespace
{
    SulDownloaderProto::ProductStatusReport_ProductStatus convert(ProductReport::ProductStatus productStatus)
    {
        using ProductStatus = ProductReport::ProductStatus;
        using namespace SulDownloaderProto;
        switch (productStatus)
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

} // namespace
bool Common::DownloadReport::operator<(
    Common::DownloadReport::ProductReport::ProductStatus lh,
    Common::DownloadReport::ProductReport::ProductStatus rh)
{
    return static_cast<int>(lh) < static_cast<int>(rh);
}

namespace Common::DownloadReport
{
    RepositoryStatus DownloadReport::getStatus() const
    {
        return m_status;
    }

    const std::string& DownloadReport::getDescription() const
    {
        return m_description;
    }

    const std::string& DownloadReport::getStartTime() const
    {
        return m_startTime;
    }

    const std::string& DownloadReport::getFinishedTime() const
    {
        return m_finishedTime;
    }



    const std::vector<ProductReport>& DownloadReport::getProducts() const
    {
        return m_productReport;
    }

    void DownloadReport::combinePreviousReportIfRequired(const DownloadReport& previousReport)
    {
        setProducts(previousReport.getProducts());
        setRepositoryComponents(previousReport.getRepositoryComponents());
    }

    void DownloadReport::setProducts(const std::vector<ProductReport>& products)
    {
        // only allow updating of products if the product list is empty.
        if (m_productReport.empty())
        {
            for (auto& product : products)
            {
                m_productReport.push_back(product);
            }
        }
    }

    void DownloadReport::setRepositoryComponents(const std::vector<ProductInfo>& repositoryComponents)
    {
        // only allow updateing of repository components if the list is empty.
        if (m_repositoryComponents.empty())
        {
            for (auto& repositoryComponent : repositoryComponents)
            {
                m_repositoryComponents.push_back(repositoryComponent);
            }
        }
    }

    const std::string& DownloadReport::getSulError() const
    {
        return m_sulError;
    }

    int DownloadReport::getExitCode() const
    {
        return static_cast<int>(m_status);
    }

    // add one return the json content directly.
    std::string DownloadReport::fromReport(const DownloadReport& report)
    {
        SulDownloaderProto::DownloadStatusReport protoReport;
        protoReport.set_starttime(report.getStartTime());
        protoReport.set_finishtime(report.getFinishedTime());

        protoReport.set_status(toString(report.getStatus()));
        protoReport.set_errordescription(report.getDescription());
        protoReport.set_sulerror(report.getSulError());
        protoReport.set_urlsource(report.getSourceURL());
        protoReport.set_supplementonlyupdate(report.isSupplementOnlyUpdate());

        for (const auto& product : report.getProducts())
        {
            SulDownloaderProto::ProductStatusReport* productReport = protoReport.add_products();
            productReport->set_productname(product.name);
            productReport->set_rigidname(product.rigidName);
            productReport->set_downloadversion(product.downloadedVersion);
            productReport->set_installedversion(product.installedVersion);
            productReport->set_errordescription(product.errorDescription);
            productReport->set_productstatus(convert(product.productStatus));
        }

        for (const auto& repositoryComponent : report.getRepositoryComponents())
        {
            SulDownloaderProto::WarehouseComponent* warehouseComponentProto = protoReport.add_warehousecomponents();
            warehouseComponentProto->set_productname(repositoryComponent.m_productName);
            warehouseComponentProto->set_rigidname(repositoryComponent.m_rigidName);
            warehouseComponentProto->set_installedversion(repositoryComponent.m_installedVersion);
            warehouseComponentProto->set_warehouseversion(repositoryComponent.m_version);
        }

        return Common::ProtobufUtil::MessageUtility::protoBuf2Json(protoReport);
    }

    DownloadReport DownloadReport::toReport(const std::string& serializedVersion)
    {
        using namespace google::protobuf::util;
        using SulDownloaderProto::DownloadStatusReport;

        DownloadStatusReport protoReport;
        JsonParseOptions jsonParseOptions;
        jsonParseOptions.ignore_unknown_fields = true;

        auto status = JsonStringToMessage(serializedVersion, &protoReport, jsonParseOptions);
        if (!status.ok())
        {
            LOGERROR("Failed to load DownloadReport serialized string");
            LOGDEBUG(status.ToString());
            throw DownloadReportParseException("Failed to load DownloadReport from string");
        }

        DownloadReport report;
        report.m_startTime = protoReport.starttime();
        report.m_finishedTime = protoReport.finishtime();

        fromString(protoReport.status(), &report.m_status);

        report.m_description = protoReport.errordescription();
        report.m_sulError = protoReport.sulerror();
        report.m_urlSource = protoReport.urlsource();

        report.m_supplementOnly = protoReport.supplementonlyupdate();

        for (auto& protoProduct : protoReport.products())
        {
            ProductReport productReport;
            productReport.rigidName = protoProduct.rigidname();
            productReport.name = protoProduct.productname();
            productReport.downloadedVersion = protoProduct.downloadversion();
            productReport.installedVersion = protoProduct.installedversion();
            productReport.errorDescription = protoProduct.errordescription();
            productReport.productStatus = convert(protoProduct.productstatus());

            report.m_productReport.push_back(productReport);
        }

        for (auto& warehouseComponentProto : protoReport.warehousecomponents())
        {
            ProductInfo productInfo;
            productInfo.m_productName = warehouseComponentProto.productname();
            productInfo.m_rigidName = warehouseComponentProto.rigidname();
            productInfo.m_version = warehouseComponentProto.warehouseversion();
            productInfo.m_installedVersion = warehouseComponentProto.installedversion();
            report.m_repositoryComponents.push_back(productInfo);
        }

        report.m_processedReport = false; // default

        return report;
    }

    std::tuple<int, std::string> DownloadReport::CodeAndSerialize(const DownloadReport& report)
    {
        std::string json = DownloadReport::fromReport(report);
        return std::tuple<int, std::string>(report.getExitCode(), json);
    }

    const std::string DownloadReport::getSourceURL() const
    {
        return m_urlSource;
    }

    const std::vector<ProductInfo>& DownloadReport::getRepositoryComponents() const
    {
        return m_repositoryComponents;
    }

    bool DownloadReport::isProcessedReport() const
    {
        return m_processedReport;
    }

    void DownloadReport::setProcessedReport(bool isProcessed)
    {
        m_processedReport = isProcessed;
    }

    bool DownloadReport::isSuccesfulProductUpdateCheck() const
    {
        if (m_supplementOnly)
        {
            return false;
        }
        return getStatus() == RepositoryStatus::SUCCESS;
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
} // namespace Common::DownloadReport
