//Copyright 2018-2023 Sophos Limited.  All rights reserved.

#include "DownloadReport.h"

#include "DownloadedProduct.h"
#include "Logger.h"
#include "SulDownloaderException.h"
#include "TimeTracker.h"

#include <ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <google/protobuf/util/json_util.h>

#include <DownloadReport.pb.h>
#include <sstream>

using namespace SulDownloader::suldownloaderdata;

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
bool SulDownloader::suldownloaderdata::operator< (SulDownloader::suldownloaderdata::ProductReport::ProductStatus lh, 
                                                    SulDownloader::suldownloaderdata::ProductReport::ProductStatus rh )
{
    return static_cast<int>(lh) < static_cast<int>(rh); 
}

namespace SulDownloader
{
    using namespace Common::UtilityImpl;

    DownloadReport DownloadReport::Report(
        const suldownloaderdata::IRepository& repository,
        const suldownloaderdata::TimeTracker& timeTracker)
    {
        DownloadReport report;
        report.setTimings(timeTracker);
        report.m_supplementOnly = false;
        if (repository.hasError())
        {
            report.setError(repository.getError());
        }
        else
        {
            report.m_status = RepositoryStatus::SUCCESS;
            report.m_description = "";
        }


        report.m_urlSource = repository.getSourceURL();
        report.m_productReport = combineProductsAndSubscriptions(
            repository.getProducts(), repository.listInstalledSubscriptions(), report.m_status);
        return report;
    }

    DownloadReport DownloadReport::Report(
        const std::string& sourceURL,
        const std::vector<suldownloaderdata::DownloadedProduct>& products,
        const std::vector<suldownloaderdata::ProductInfo>& components,
        const std::vector<suldownloaderdata::SubscriptionInfo>& subscriptionsToALCStatus,
        TimeTracker* timeTracker,
        VerifyState verifyState,
        bool supplementOnly,
        bool baseDowngrade)
    {
        assert(timeTracker != nullptr);

        DownloadReport report;

        report.m_status = RepositoryStatus::SUCCESS;
        report.m_description = "";
        report.m_urlSource = sourceURL;
        report.m_supplementOnly = supplementOnly;
        report.m_baseDowngrade = baseDowngrade;

        if (products.empty())
        {
            // empty list means no products should have been downloaded.
            report.m_status = RepositoryStatus::DOWNLOADFAILED;
        }

        for (const auto& product : products)
        {
            if (product.hasError())
            {
                if (!product.getProductIsBeingUninstalled())
                {
                    report.m_description = "Update failed";
                    report.m_status = RepositoryStatus::INSTALLFAILED;
                    break; // Install failures are a higher error than uninstalled.
                }
                else
                {
                    report.m_description = "Uninstall failed";
                    report.m_status = RepositoryStatus::UNINSTALLFAILED;
                }
            }
        }

        report.m_productReport = combineProductsAndSubscriptions(products, subscriptionsToALCStatus, report.m_status);

        if (verifyState == VerifyState::VerifyFailed)
        {
            for (auto& productReport : report.m_productReport)
            {
                productReport.productStatus = ProductReport::ProductStatus::VerifyFailed;
            }
        }

        if (report.m_status == RepositoryStatus::SUCCESS)
        {
            timeTracker->setSyncTime();
        }

        report.setTimings(*timeTracker);

        report.m_repositoryComponents = updateRepositoryComponentInstalledVersion(components);

        return report;
    }

    DownloadReport DownloadReport::Report(const std::string& errorDescription)
    {
        DownloadReport report;
        TimeTracker tt;
        tt.setStartTime(TimeUtils::getCurrTime());
        report.setTimings(tt);
        report.m_description = errorDescription;
        report.m_status = RepositoryStatus::UNSPECIFIED;
        return report;
    }

    std::vector<std::string> DownloadReport::listOfAllPreviousReports(const std::string& outputParentPath)
    {
        std::vector<std::string> filesInReportDirectory = Common::FileSystem::fileSystem()->listFiles(outputParentPath);

        // Filter file list to make sure all the files are report files based on file name
        std::vector<std::string> previousReportFiles;
        std::string startPattern("update_report");
        std::string endPattern(".json");

        for (auto& file : filesInReportDirectory)
        {
            std::string fileName = Common::FileSystem::basename(file);

            // make sure file name begins with 'report' and ends with '.json'
            using strUtils = Common::UtilityImpl::StringUtils;

            if (strUtils::startswith(fileName, startPattern) && strUtils::endswith(fileName, endPattern))
            {
                previousReportFiles.emplace_back(file);
            }
        }
        return previousReportFiles;
    }

    RepositoryStatus DownloadReport::getStatus() const { return m_status; }

    const std::string& DownloadReport::getDescription() const { return m_description; }

    const std::string& DownloadReport::getStartTime() const { return m_startTime; }

    const std::string& DownloadReport::getFinishedTime() const { return m_finishedTime; }

    const std::string& DownloadReport::getSyncTime() const { return m_sync_time; }

    const std::vector<ProductReport>& DownloadReport::getProducts() const { return m_productReport; }

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

    const std::vector<ProductInfo> DownloadReport::updateRepositoryComponentInstalledVersion(const std::vector<ProductInfo>& repositoryComponents)
    {
        std::vector<ProductInfo> updatedRepositoryComponents;

        for(auto& repositoryComponent : repositoryComponents)
        {
            ProductInfo repositoryProductInfo;
            repositoryProductInfo.m_rigidName = repositoryComponent.m_rigidName;
            repositoryProductInfo.m_productName = repositoryComponent.m_productName;
            repositoryProductInfo.m_version = repositoryComponent.m_version;
            repositoryProductInfo.m_installedVersion = getInstalledVersion(repositoryComponent.m_rigidName);
            updatedRepositoryComponents.push_back(repositoryProductInfo);
        }
        return updatedRepositoryComponents;
    }

    std::vector<ProductReport> DownloadReport::combineProductsAndSubscriptions(
        const std::vector<suldownloaderdata::DownloadedProduct>& products,
        const std::vector<suldownloaderdata::SubscriptionInfo>& subscriptionsInfo,
        const RepositoryStatus& repositoryStatus)
    {
        std::unordered_map<std::string, ProductReport> productReport;
        std::vector<ProductReport> productsRep;
        for (const auto& product : products)
        {
            ProductReport productReportEntry;
            const auto& info = product.getProductMetadata();
            productReportEntry.rigidName = info.getLine();
            productReportEntry.name = info.getName();
            productReportEntry.downloadedVersion = info.getVersion();
            productReportEntry.installedVersion = getInstalledVersion(productReportEntry.rigidName);
            auto wError = product.getError();
            productReportEntry.errorDescription = wError.Description;

            // Set product status appropriately
            if ((repositoryStatus == RepositoryStatus::DOWNLOADFAILED) ||
                (repositoryStatus == RepositoryStatus::PACKAGESOURCEMISSING))
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
            LOGDEBUG("Product Report for product downloaded: " << productReportEntry.rigidName << " " << productReportEntry.statusToString() << " err: " << productReportEntry.errorDescription);

            productReport[productReportEntry.rigidName] = productReportEntry;
            if (productReportEntry.productStatus == ProductReport::ProductStatus::Uninstalled || productReportEntry.productStatus == ProductReport::ProductStatus::UninstallFailed)
            {
                auto productMatchSubscriptionEntry = [&productReportEntry](const SubscriptionInfo & sI){ return sI.rigidName == productReportEntry.rigidName; }; 
                if ( std::find_if(subscriptionsInfo.begin(), subscriptionsInfo.end(), productMatchSubscriptionEntry) == subscriptionsInfo.end() )
                {
                    // uninstalled products will not be in the subscription info, hence, explicitly adding it
                    LOGDEBUG("Adding information for uninstalled products directly to the combined list " << productReportEntry.rigidName << " " << productReportEntry.statusToString() << " err: " << productReportEntry.errorDescription);
                    productsRep.push_back(productReportEntry);
                }
            }
        }
        LOGDEBUG("Combine the products to merge into the subscriptions");
        
        for (const auto& subscriptionInfo : subscriptionsInfo)
        {
            auto found = productReport.find(subscriptionInfo.rigidName);
            if (found != productReport.end())
            {
                LOGDEBUG("Product Report merged to subscription: " << found->second.rigidName << " " << found->second.statusToString() << " err: " << found->second.errorDescription);
                auto entry = found->second;
                entry.downloadedVersion = subscriptionInfo.version;
                productsRep.push_back(entry);
            }
            else
            {
                ProductReport productReportEntry;
                productReportEntry.rigidName = subscriptionInfo.rigidName;
                productReportEntry.downloadedVersion = subscriptionInfo.version;
                productReportEntry.name = subscriptionInfo.rigidName;
                productReportEntry.installedVersion = getInstalledVersion( productReportEntry.rigidName);
                std::string combinedError;
                ProductReport::ProductStatus combinedStatus{ ProductReport::ProductStatus::UpToDate };

                for (auto& subComp : subscriptionInfo.subProducts)
                {
                    auto subComponentProduct = productReport.find(subComp.m_line);

                    if (subComponentProduct != productReport.end())
                    {
                        const std::string& error{ subComponentProduct->second.errorDescription };
                        suldownloaderdata::ProductReport::ProductStatus status =
                            subComponentProduct->second.productStatus;

                        /*
                         * if sub component has own installer, create a product report entry for the component
                         * as well as the combined entry.
                         */
                        for (auto& product : products)
                        {
                            if (product.getLine() == subComp.m_line)
                            {
                                if (Common::UtilityImpl::StringUtils::isSubstring(product.installerPath(), subComp.m_line))
                                {
                                    // installer located in sub component.
                                    LOGDEBUG("Added subcomponent to report entry for :" << subComp.m_line);
                                    ProductReport subProductReportEntry;
                                    subProductReportEntry.rigidName = subComp.m_line;
                                    subProductReportEntry.downloadedVersion = subComp.m_version;
                                    subProductReportEntry.installedVersion = getInstalledVersion(subProductReportEntry.rigidName);
                                    subProductReportEntry.name = subComp.m_line;
                                    subProductReportEntry.productStatus = subComponentProduct->second.productStatus;
                                    subProductReportEntry.errorDescription = product.getError().Description;
                                    productsRep.push_back(subProductReportEntry);
                                }
                                break;
                            }
                        }

                        if (!error.empty())
                        {
                            if (combinedError.empty())
                            {
                                combinedError = error;
                            }
                            else
                            {
                                combinedError += ". " + error;
                            }
                        }
                        combinedStatus = std::max(combinedStatus, status);
                    }
                }
                productReportEntry.productStatus = combinedStatus;
                productReportEntry.errorDescription = combinedError;
                LOGDEBUG("Product Report merged to subscription: " << productReportEntry.rigidName << " " << productReportEntry.statusToString() << " err: " << productReportEntry.errorDescription);

                productsRep.push_back(productReportEntry);
            }
        }

        return productsRep;
    }

    void DownloadReport::setTimings(const TimeTracker& timeTracker)
    {
        m_startTime = timeTracker.startTime();
        m_sync_time = timeTracker.syncTime();
        m_finishedTime = timeTracker.finishedTime();
    }

    const std::string& DownloadReport::getSulError() const { return m_sulError; }

    void DownloadReport::setError(const RepositoryError& error)
    {
        m_description = error.Description;
        m_status = error.status;
        m_sulError = error.LibError;
    }

    int DownloadReport::getExitCode() const { return static_cast<int>(m_status); }

    // add one return the json content directly.
    std::string DownloadReport::fromReport(const DownloadReport& report)
    {
        SulDownloaderProto::DownloadStatusReport protoReport;
        protoReport.set_starttime(report.getStartTime());
        protoReport.set_finishtime(report.getFinishedTime());
        protoReport.set_synctime(report.getSyncTime());

        protoReport.set_status(toString(report.getStatus()));
        protoReport.set_errordescription(report.getDescription());
        protoReport.set_sulerror(report.getSulError());
        protoReport.set_urlsource(report.getSourceURL());
        protoReport.set_supplementonlyupdate(report.m_supplementOnly);

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
            LOGERROR("Failed to load SulDownload serialized string");
            LOGSUPPORT(status.ToString());
            throw SulDownloaderException("Failed to load SulDownloadReport from string");
        }

        DownloadReport report;
        report.m_startTime = protoReport.starttime();
        report.m_finishedTime = protoReport.finishtime();
        report.m_sync_time = protoReport.synctime();

        fromString(protoReport.status(), &report.m_status);

        report.m_description = protoReport.errordescription();
        report.m_sulError = protoReport.sulerror();
        report.m_urlSource = protoReport.urlsource();

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

    const std::string DownloadReport::getSourceURL() const { return m_urlSource; }

    const std::vector<ProductInfo>& DownloadReport::getRepositoryComponents() const { return m_repositoryComponents; }

    bool DownloadReport::isProcessedReport() const { return m_processedReport; }

    void DownloadReport::setProcessedReport(bool isProcessed) { m_processedReport = isProcessed; }

    bool DownloadReport::isSuccesfulProductUpdateCheck() const
    {
        if (m_supplementOnly)
        {
            return false;
        }
        return getStatus() == SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS;
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

    std::string DownloadReport::getInstalledVersion(const std::string& rigidName)
    {
        std::string localVersionIni =
            Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(rigidName);
        try
        {
            return StringUtils::extractValueFromIniFile(localVersionIni, "PRODUCT_VERSION");
        }
        catch (const std::exception& ex)
        {
            LOGDEBUG("Unable to read version for \"" << rigidName << "\" err: " << ex.what());
        }
        return "";  // return an empty string if installed version not found
    }

} // namespace SulDownloader