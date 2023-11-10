// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "DownloadReportBuilder.h"

#include "DownloadedProduct.h"
#include "Logger.h"

#include "Common/DownloadReport/DownloadReport.h"
#include "Common/DownloadReport/ProductInfo.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <cassert>

using namespace Common::DownloadReport;
using namespace Common::UtilityImpl;

namespace SulDownloader::suldownloaderdata
{
    DownloadReport DownloadReportBuilder::Report(
        const suldownloaderdata::IRepository& repository,
        const suldownloaderdata::TimeTracker& timeTracker)
    {
        DownloadReport report;
        report.m_startTime = timeTracker.startTime();
        report.m_finishedTime = timeTracker.finishedTime();
        report.m_supplementOnly = false;
        if (repository.hasError())
        {
            const auto& error = repository.getError();
            report.m_description = error.Description;
            report.m_status = error.status;
            report.m_sulError = error.LibError;
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

    DownloadReport DownloadReportBuilder::Report(
        const std::string& sourceURL,
        const std::vector<suldownloaderdata::DownloadedProduct>& products,
        const std::vector<ProductInfo>& components,
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

        report.m_startTime = timeTracker->startTime();
        report.m_finishedTime = timeTracker->finishedTime();

        report.m_repositoryComponents = updateRepositoryComponentInstalledVersion(components);

        return report;
    }

    DownloadReport DownloadReportBuilder::Report(const std::string& errorDescription)
    {
        DownloadReport report;
        TimeTracker tt;
        tt.setStartTime(TimeUtils::getCurrTime());
        report.m_startTime = tt.startTime();
        report.m_finishedTime = tt.finishedTime();
        report.m_description = errorDescription;
        report.m_status = RepositoryStatus::UNSPECIFIED;
        return report;
    }

    const std::vector<ProductInfo> DownloadReportBuilder::updateRepositoryComponentInstalledVersion(
        const std::vector<ProductInfo>& repositoryComponents)
    {
        std::vector<ProductInfo> updatedRepositoryComponents;

        for (auto& repositoryComponent : repositoryComponents)
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

    std::vector<ProductReport> DownloadReportBuilder::combineProductsAndSubscriptions(
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
            LOGDEBUG(
                "Product Report for product downloaded: " << productReportEntry.rigidName << " "
                                                          << productReportEntry.statusToString()
                                                          << " err: " << productReportEntry.errorDescription);

            productReport[productReportEntry.rigidName] = productReportEntry;
            if (productReportEntry.productStatus == ProductReport::ProductStatus::Uninstalled ||
                productReportEntry.productStatus == ProductReport::ProductStatus::UninstallFailed)
            {
                auto productMatchSubscriptionEntry = [&productReportEntry](const SubscriptionInfo& sI)
                { return sI.rigidName == productReportEntry.rigidName; };
                if (std::find_if(subscriptionsInfo.begin(), subscriptionsInfo.end(), productMatchSubscriptionEntry) ==
                    subscriptionsInfo.end())
                {
                    // uninstalled products will not be in the subscription info, hence, explicitly adding it
                    LOGDEBUG(
                        "Adding information for uninstalled products directly to the combined list "
                        << productReportEntry.rigidName << " " << productReportEntry.statusToString()
                        << " err: " << productReportEntry.errorDescription);
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
                LOGDEBUG(
                    "Product Report merged to subscription: " << found->second.rigidName << " "
                                                              << found->second.statusToString()
                                                              << " err: " << found->second.errorDescription);
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
                productReportEntry.installedVersion = getInstalledVersion(productReportEntry.rigidName);
                std::string combinedError;
                ProductReport::ProductStatus combinedStatus{ ProductReport::ProductStatus::UpToDate };

                for (auto& subComp : subscriptionInfo.subProducts)
                {
                    auto subComponentProduct = productReport.find(subComp.m_line);

                    if (subComponentProduct != productReport.end())
                    {
                        const std::string& error{ subComponentProduct->second.errorDescription };
                        ProductReport::ProductStatus status = subComponentProduct->second.productStatus;

                        /*
                         * if sub component has own installer, create a product report entry for the component
                         * as well as the combined entry.
                         */
                        for (auto& product : products)
                        {
                            if (product.getLine() == subComp.m_line)
                            {
                                if (Common::UtilityImpl::StringUtils::isSubstring(
                                        product.installerPath(), subComp.m_line))
                                {
                                    // installer located in sub component.
                                    LOGDEBUG("Added subcomponent to report entry for :" << subComp.m_line);
                                    ProductReport subProductReportEntry;
                                    subProductReportEntry.rigidName = subComp.m_line;
                                    subProductReportEntry.downloadedVersion = subComp.m_version;
                                    subProductReportEntry.installedVersion =
                                        getInstalledVersion(subProductReportEntry.rigidName);
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
                LOGDEBUG(
                    "Product Report merged to subscription: " << productReportEntry.rigidName << " "
                                                              << productReportEntry.statusToString()
                                                              << " err: " << productReportEntry.errorDescription);

                productsRep.push_back(productReportEntry);
            }
        }

        return productsRep;
    }

    std::string DownloadReportBuilder::getInstalledVersion(const std::string& rigidName)
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
        return ""; // return an empty string if installed version not found
    }
} // namespace SulDownloader::suldownloaderdata
