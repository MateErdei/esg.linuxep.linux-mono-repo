/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IPidLockFileUtils.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <SulDownloader/Logger.h>
#include <SulDownloader/ProductUninstaller.h>
#include <SulDownloader/SulDownloader.h>
#include <SulDownloader/sdds3/SDDS3Utils.h>
#include <SulDownloader/sdds3/Sdds3RepositoryFactory.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/ConfigurationDataUtil.h>
#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <SulDownloader/suldownloaderdata/TimeTracker.h>
#include <SulDownloader/warehouse/SULRaii.h>
#include <SulDownloader/warehouse/WarehouseRepository.h>
#include <SulDownloader/warehouse/WarehouseRepositoryFactory.h>

#include <algorithm>
#include <cassert>
#include <sys/stat.h>
#include <thread>

using namespace SulDownloader::suldownloaderdata;

namespace
{
    bool hasError(const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>& products)
    {
        for (const auto& product : products)
        {
            if (product.hasError())
            {
                return true;
            }
        }
        return false;
    }

} // namespace

namespace SulDownloader
{
    using namespace Common::UtilityImpl;

    bool forceInstallOfProduct(const DownloadedProduct& product, const DownloadReport& previousDownloadReport)
    {
        // If the previous install failed with unspecified or download failedwe have no clear
        // information on the state of the distribution folder. Therefore safest approach is to force a reinstall.
        // We also need to force re-install if the list of products and warehouse components are empty
        // this means we are in a state which could result in reporting success, when an install failed previously
        // due to loosing this information on connection failure.
        if (previousDownloadReport.getStatus() == RepositoryStatus::UNSPECIFIED ||
            previousDownloadReport.getStatus() == RepositoryStatus::DOWNLOADFAILED ||
            (previousDownloadReport.getStatus() == RepositoryStatus::CONNECTIONERROR &&
             previousDownloadReport.getProducts().empty() && previousDownloadReport.getRepositoryComponents().empty()))
        {
            LOGDEBUG("Force reinstall because previous report failed.");
            return true;
        }

        const std::vector<ProductReport>& productReports = previousDownloadReport.getProducts();
        auto productReportItr =
            std::find_if(productReports.begin(), productReports.end(), [&product](const ProductReport& report) {
                return report.rigidName == product.getLine();
            });

        if (productReportItr != productReports.end())
        {
            if (productReportItr->productStatus == ProductReport::ProductStatus::InstallFailed ||
                productReportItr->productStatus == ProductReport::ProductStatus::SyncFailed ||
                productReportItr->productStatus == ProductReport::ProductStatus::VerifyFailed)
            {
                LOGDEBUG("Force reinstall because this specific product failed.");
                return true;
            }
        }
        return false;
    }

    static bool internal_runSULDownloader(
        IWarehouseRepositoryPtr& warehouseRepository,
        const ConfigurationData& configurationData,
        const suldownloaderdata::ConnectionSetup& connectionSetup,
        bool supplementOnly)
    {
        warehouseRepository->reset();
        warehouseRepository->tryConnect(connectionSetup, supplementOnly, configurationData);

        if (warehouseRepository->hasError())
        {
            LOGDEBUG("Failed to connect to warehouse: " << warehouseRepository->getError().Description);
            return false;
        }

        // apply product selection and download the needed products
        auto productSelection = ProductSelection::CreateProductSelection(configurationData);
        warehouseRepository->synchronize(productSelection);

        if (warehouseRepository->hasError())
        {
            LOGDEBUG("Failed to synchronise warehouse: " << warehouseRepository->getError().Description);
            return false;
        }

        warehouseRepository->distribute();
        if (warehouseRepository->hasError())
        {
            LOGDEBUG("Failed to distribute warehouse: " << warehouseRepository->getError().Description);
            return false;
        }

        return true;
    }

    static bool isImmediateFailure(const IWarehouseRepositoryPtr& warehouseRepository)
    {
        return warehouseRepository->hasImmediateFailError();
    }

    suldownloaderdata::DownloadReport processRepositoryAndGenerateReport(const bool success,
                                                                         IRepositoryPtr repository,
                                                                         TimeTracker& timeTracker,
                                                                         const ConfigurationData& configurationData,
                                                                         const DownloadReport& previousDownloadReport,
                                                                         const bool forceReinstallAllProducts,
                                                                         const bool supplementOnly)
    {
        if (!success)
        {
            DownloadReport report = DownloadReport::Report(*repository, timeTracker);
            if(report.getProducts().empty() && report.getRepositoryComponents().empty())
            {
                // Populate report products warehouse components from previous report, so that any issues from previously update are
                // carried over into the next update.
                report.combinePreviousReportIfRequired(previousDownloadReport);
            }
            return report;
        }

        assert(success);

        auto products = repository->getProducts();
        std::string sourceURL = repository->getSourceURL();

        for (auto& product : products)
        {
            std::string rigidName = product.getProductMetadata().getLine();
            std::string warehouseVersionIni = Common::FileSystem::join(product.distributePath(), "VERSION.ini");
            LOGDEBUG("Checking if " << rigidName << " needs to be downgraded");

            try
            {
                std::string localVersionIni =
                    Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(rigidName);
                if (!Common::FileSystem::fileSystem()->isFile(localVersionIni))
                {
                    // if local version.ini doesn't exist assume plugin is an older version than XDR EAP or not
                    // installed therefore no downgrade
                    LOGDEBUG("Plugin " << rigidName << " in warehouse is newer than version on disk");
                }
                else
                {
                    std::string currentVersion =
                        StringUtils::extractValueFromIniFile(localVersionIni, "PRODUCT_VERSION");
                    std::string newVersion("");
                    try
                    {
                        newVersion = StringUtils::extractValueFromIniFile(warehouseVersionIni, "PRODUCT_VERSION");
                    }
                    catch (std::runtime_error& ex)
                    {
                        LOGINFO(
                            "Failed to read VERSION.ini from warehouse for: '" << rigidName
                                                                               << "', treating as downgrade");
                        product.setProductWillBeDowngraded(true);
                    }

                    if (newVersion.empty() || currentVersion.empty())
                    {
                        // if newVersion is empty could be due the product not being able to read the file
                        // if currentVersion is empty something went wrong with the install possibly
                        // therefore we would like to do a downgrade to get as close to a fresh install as possible
                        product.setProductWillBeDowngraded(true);
                    }
                    else
                    {
                        bool willBeDowngraded = StringUtils::isVersionOlder(currentVersion, newVersion);
                        if (willBeDowngraded)
                        {
                            LOGDEBUG("Component " << rigidName << " is being downgraded");
                        }
                        else
                        {
                            LOGDEBUG("Component " << rigidName << " is being updated");
                        }
                        product.setProductWillBeDowngraded(willBeDowngraded);
                    }
                }
            }
            catch (std::runtime_error& ex)
            {
                // run time error thrown if local version.ini file missing
                LOGWARN(ex.what());
            }
            catch (std::invalid_argument& ex)
            {
                // invalid argument thrown by isVersionOlder if Version string are not in the right format
                LOGWARN("Failed to calculate version difference due to: " << ex.what());
            }
        }
        bool productChanging = false;
        for (auto& product : products)
        {
            bool forceReinstallThisProduct = forceInstallOfProduct(product, previousDownloadReport);

            if (forceReinstallAllProducts || forceReinstallThisProduct)
            {
                LOGSUPPORT(
                    "Mark product to be reinstalled. Reason: AllProducts: "
                        << forceReinstallAllProducts << ", This Product: " << forceReinstallThisProduct
                        << ". Product = " << product.getLine());
                product.setForceProductReinstall(true);
                productChanging = true;
            }
        }
        LOGSUPPORT("Checking signature.");
        // Only need to verify the products which the install.sh will be called on.
        for (auto& product : products)
        {
            if (product.productHasChanged() || product.forceProductReinstall() || product.productWillBeDowngraded())
            {
                product.verify(configurationData);

                productChanging = true;
            }
        }
        if (hasError(products))
        {
            LOGWARN("Verification of the downloaded products failed.");
            return DownloadReport::Report(
                sourceURL,
                products,
                {},
                repository->listInstalledSubscriptions(),
                &timeTracker,
                DownloadReport::VerifyState::VerifyFailed,
                false);
        }

        auto fileSystem = Common::FileSystem::fileSystem();
        if (productChanging)
        {
            std::string markerPath = Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
            std::string currentWorking = fileSystem->currentWorkingDirectory();
            try
            {
                // if current working dir is slash then Suldownloader has been started from systemd
                if (currentWorking == "/")
                {
                    fileSystem->writeFile(markerPath, "");
                    auto fp = Common::FileSystem::filePermissions();
                    fp->chown(markerPath, sophos::updateSchedulerUser(), sophos::group());
                    fp->chmod(markerPath, S_IRUSR | S_IWUSR | S_IRGRP);
                }
                else
                {
                    fileSystem->writeFile(markerPath, currentWorking);
                }
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to create update marker file due to error: " << ex.what());
            }

        }
        // Note: Should only get here if Download has been successful, if no products are downloaded then
        // a warehouse error should have been generated, preventing getting this far, therefore preventing
        // un-installation of all products.
        // We need to perform un-install of any components not downloaded first to prevent a race condition
        // between component registration and product un-installation when upgrading Base.
        SulDownloader::ProductUninstaller uninstallManager;
        std::vector<DownloadedProduct> uninstalledProducts =
            uninstallManager.removeProductsNotDownloaded(products, *repository);
        for (auto& uninstalledProduct : uninstalledProducts)
        {
            products.push_back(uninstalledProduct);
        }

        // design decision: do not install / reinstall if any error happens before this time.
        int productIndex = 0;
        bool setForceInstallForAllProducts = false;
        for (auto& product : products)
        {
            if (product.productWillBeDowngraded())
            {
                // Base should always be the first product in the list.
                // if base is being downgraded, then all products will be treated as a downgrade
                // therefore only need to run the base uninstaller.
                // if base is not being downgraded, then need to run the uninstaller for each component.
                if (productIndex == 0)
                {
                    LOGDEBUG("Preparing " << product.getLine() << " for downgrade");
                    std::string baseUninstallerPath =
                        Common::ApplicationConfiguration::applicationPathManager().getLocalBaseUninstallerPath();
                    uninstallManager.prepareProductForDowngrade(baseUninstallerPath);
                    LOGINFO("Prepared " << product.getLine() << " for downgrade");
                    setForceInstallForAllProducts = true;
                }
                else
                {
                    if (!setForceInstallForAllProducts)
                    {
                        LOGDEBUG("Preparing " << product.getLine() << " for downgrade");
                        std::string componentPath = Common::FileSystem::join(
                            Common::ApplicationConfiguration::applicationPathManager().getLocalUninstallSymLinkPath(),
                            product.getLine() + ".sh");
                        uninstallManager.prepareProductForDowngrade(componentPath);
                        LOGINFO("Prepared " << product.getLine() << " for downgrade");
                        product.setForceProductReinstall(true);
                    }
                }
            }
            if (setForceInstallForAllProducts)
            {
                product.setForceProductReinstall(true);
            }
            productIndex++;
        }

        // try to install all products and report error for those that failed (if any)

        for (auto& product : products)
        {
            if ( (product.productHasChanged() || product.forceProductReinstall()) && !product.getProductIsBeingUninstalled())
            {
                product.install(configurationData.getInstallArguments());
            }
            else
            {
                LOGINFO("Downloaded Product line: '" << product.getLine() << "' is up to date.");
            }
        }

        timeTracker.setFinishedTime(TimeUtils::getCurrTime());
        if (productChanging)
        {
            try
            {
                std::string updateMarkerFile = Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
                if (fileSystem->isFile(updateMarkerFile))
                {
                    fileSystem->removeFile(updateMarkerFile);
                }
                else
                {
                    LOGWARN("Marker file removed before update finished");
                }
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to remove marker file with error: " << ex.what());
            }
        }

        // if any error happened during installation, it reports correctly.
        // the report also contains the successful ones.

        LOGDEBUG("Triggering purge");
        repository->purge();
        return DownloadReport::Report(
            sourceURL,
            products,
            repository->listInstalledProducts(),
            repository->listInstalledSubscriptions(),
            &timeTracker,
            DownloadReport::VerifyState::VerifyCorrect,
            supplementOnly,
            setForceInstallForAllProducts);
    }

    std::vector<ConnectionSetup> populateSdds3ConnectionCandidates(const ConfigurationData& configurationData)
    {
        ConnectionSelector connectionSelector;
        auto candidates = connectionSelector.getConnectionCandidates(configurationData);
        std::vector<ConnectionSetup> finalConnectionCandidates;

        std::vector<Proxy> proxies;
        // get list of possible proxies
        for (auto& candidate : candidates)
        {
            if(!candidate.isCacheUpdate() && !candidate.getProxy().empty())
            {
                proxies.push_back(candidate.getProxy());
            }
        }
        std::string sdds3OverrideSettingsFile = Common::ApplicationConfiguration::applicationPathManager().getSdds3OverrideSettingsFile();
        std::string overrideValue;
        if (Common::FileSystem::fileSystem()->isFile(sdds3OverrideSettingsFile))
        {
            overrideValue = StringUtils::extractValueFromIniFile(sdds3OverrideSettingsFile, "URLS");
        }

        std::vector<std::string> urls = {"https://sus.sophosupd.com"};
        if (!overrideValue.empty())
        {
            LOGWARN("Overriding Sophos Update Service address list with " << overrideValue);
            urls = StringUtils::splitString(overrideValue, ",");
        }

        for (const auto& proxy : proxies)
        {
            for (const auto& url : urls)
            {
                Credentials credentials;
                ConnectionSetup connectionSetup(url, credentials, false, proxy);
                finalConnectionCandidates.emplace_back(connectionSetup);
                LOGDEBUG("Adding SDDS3 connection candidate, URL: " << url << ", proxy: " << proxy.getUrl());
            }
        }

        for (const auto& url : urls)
        {
            Credentials credentials;
            ConnectionSetup connectionSetup(url, credentials, false, Proxy());
            finalConnectionCandidates.push_back(connectionSetup);
            LOGDEBUG("Adding SDDS3 connection candidate, URL: " << url);
        }

        return finalConnectionCandidates;
    }

    void createSdds3UpdateCacheFolders()
    {
        std::string sdds3DistributionPath =
            Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository();
        std::string sdds3RepositoryPath =
            Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository();
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            if (!fileSystem->exists(sdds3RepositoryPath))
            {
                fileSystem->makedirs(sdds3RepositoryPath);
            }
            if (!fileSystem->exists(sdds3DistributionPath))
            {
                fileSystem->makedirs(sdds3DistributionPath);
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to create sdds3 local repository paths, error: " << ex.what());
        }
    }

    std::pair<bool, IRepositoryPtr> updateFromSDDS3Repository( const ConfigurationData& configurationData,
                                                              bool supplementOnly)
    {
        auto candidates = populateSdds3ConnectionCandidates(configurationData);
        auto repository = Sdds3RepositoryFactory::instance().createRepository();

        for (auto& connectionCandidate : candidates)
        {
            LOGDEBUG("Trying connection candidate, URL: " << connectionCandidate.getUpdateLocationURL() << ", proxy: " << connectionCandidate.getProxy().getUrl());
            if (repository->tryConnect(connectionCandidate, supplementOnly, configurationData))
            {
                break;
            }
        }

        if(repository->hasError())
        {
            LOGERROR("Failed to connect to repository: " << repository->getError().Description);
            return std::make_pair(false, std::move(repository));
        }

        LOGDEBUG("Purging local SDDS2 cache");
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            if (fileSystem->isDirectory(configurationData.getLocalWarehouseRepository()))
            {
                fileSystem->recursivelyDeleteContentsOfDirectory(configurationData.getLocalWarehouseRepository());
            }
            if (fileSystem->isDirectory(configurationData.getLocalDistributionRepository()))
            {
                fileSystem->recursivelyDeleteContentsOfDirectory(configurationData.getLocalDistributionRepository());
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to delete SDDS2 cache, reason:" << ex.what());
        }

        repository->synchronize(configurationData);

        if(repository->hasError())
        {
            LOGERROR("Failed to synchronize repository: " << repository->getError().Description);
            return std::make_pair(false, std::move(repository));
        }

        repository->distribute();
        if(repository->hasError())
        {
            LOGERROR("Failed to distribute repository: " << repository->getError().Description);
            return std::make_pair(false, std::move(repository));
        }

        return std::make_pair(true, std::move(repository));
    }
    std::pair<bool, IRepositoryPtr> updateFromSDDS2Warehouse( const ConfigurationData& configurationData,
                                                              bool supplementOnly)
    {

        SULInit init;

        auto warehouseRepository = WarehouseRepositoryFactory::instance().createWarehouseRepository();
        assert(warehouseRepository);

        bool success = false;

        ConnectionSelector connectionSelector;
        auto candidates = connectionSelector.getConnectionCandidates(configurationData);

        removeSDDS3Cache();

        for (const auto& connectionSetup : candidates)
        {
            success =
                internal_runSULDownloader(warehouseRepository, configurationData, connectionSetup, supplementOnly);
            if (success)
            {
                LOGDEBUG("Successfully ran SUL Downloader");
                break;
            }
            else if (isImmediateFailure(warehouseRepository))
            {
                // Immediate failures: currently UPDATESOURCEMISSING
                // Currently no immediate failures are possible for supplement-only updating - but need to abort if
                // supplementOnly=False
                assert(!supplementOnly); // currently never-supplement only - change message if this changes
                LOGERROR("Immediate failure of updating");
                break; // would still try updating products, if it were supplementOnly
            }
        }

        if (supplementOnly && !success)
        {
            LOGINFO("Retry with product update, in case the supplement config has changed");
            // retry with product update, in case the supplement config has changed
            // also if we get an immediate failure from the supplement-only update, we retry product-update
            // (This can't happen currently, since the only immediate failure can't happen with local meta-data)
            supplementOnly = false;
            for (const auto& connectionSetup : candidates)
            {
                success =
                    internal_runSULDownloader(warehouseRepository, configurationData, connectionSetup, supplementOnly);
                if (success)
                {
                    LOGDEBUG("Successfully ran SUL Downloader");
                    break;
                }
                else if (isImmediateFailure(warehouseRepository))
                {
                    // Immediate failures: currently UPDATESOURCEMISSING
                    LOGERROR("Immediate failure of updating");
                    break;
                }
            }
        }

        if(!success)
        {
            // Failed to download from SDDS
            warehouseRepository->dumpLogs();
        }

        return std::make_pair(success, std::move(warehouseRepository));
    }

    DownloadReport runSULDownloader(
        const ConfigurationData& configurationData,
        const ConfigurationData& previousConfigurationData,
        const DownloadReport& previousDownloadReport,
        bool supplementOnly)
    {
        // Mark which products need to be forced to re/install.
        bool forceReinstallAllProducts =
            SulDownloader::suldownloaderdata::ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(
                configurationData, previousConfigurationData, false);
        assert(configurationData.isVerified());

        if (supplementOnly)
        {
            LOGINFO("Doing supplement-only update");
        }
        else
        {
            LOGINFO("Doing product and supplement update");
        }

        // connect and read metadata
        if (forceReinstallAllProducts)
        {
            // If we need to reinstall products, then we can't do a supplement-only update
            LOGINFO("Forcing product update due previous update failure or change in configuration");
            supplementOnly = false;
        }

        TimeTracker timeTracker;
        timeTracker.setStartTime(TimeUtils::getCurrTime());

        std::pair<bool, IRepositoryPtr> repositoryResult;
        std::string overrideFile = Common::ApplicationConfiguration::applicationPathManager().getSdds3OverrideSettingsFile();
        bool useSdds3 = configurationData.getUseSDDS3();
        if (!useSdds3 && Common::FileSystem::fileSystem()->exists(overrideFile))
        {
            useSdds3 = !(StringUtils::extractValueFromIniFile(overrideFile, "USE_SDDS3").empty());
        }

        if (useSdds3 && !configurationData.getPrimarySubscription().fixedVersion().empty())
        {
            if (StringUtils::isVersionOlder("1.2", configurationData.getPrimarySubscription().fixedVersion()))
            {
                LOGINFO(
                    "The requested fixed version is not available on SDDS3: " +
                    configurationData.getPrimarySubscription().fixedVersion() + ". Reverting to SDDS2 mode.");
                useSdds3 = false;
            }
        }

        if (!configurationData.getJWToken().empty() && useSdds3)
        {
            LOGINFO("Running in SDDS3 updating mode");
            // Make sure root directories are created
            createSdds3UpdateCacheFolders();
            repositoryResult = updateFromSDDS3Repository(configurationData, supplementOnly);
        }
        else
        {
            LOGINFO("Running in SDDS2 updating mode");
            repositoryResult = updateFromSDDS2Warehouse(configurationData, supplementOnly);
        }

        return processRepositoryAndGenerateReport(repositoryResult.first,
                                                  std::move(repositoryResult.second),
                                                  timeTracker,
                                                  configurationData,
                                                  previousDownloadReport,
                                                  forceReinstallAllProducts,
                                                  supplementOnly);

    }


    std::tuple<int, std::string, bool> configAndRunDownloader(
        const std::string& inputFilePath,
        const std::string& previousInputFilePath,
        const std::string& previousReportData,
        bool supplementOnly)
    {
        bool readSuccessful = false;
        try
        {
            int readAttempt = 0;
            int maxReadAttempt = 10;
            auto fileSystem  = Common::FileSystem::fileSystem();

            ConfigurationData configurationData;
            ConfigurationData previousConfigurationData;
            do
            {
                readAttempt++;
                try
                {
                    std::string settingsString = fileSystem->readFile(inputFilePath);
                    LOGDEBUG("Configure and run downloader: " << settingsString);

                    configurationData = ConfigurationData::fromJsonSettings(settingsString);

                    if (fileSystem->isFile(previousInputFilePath))
                    {
                        std::string previousSettingString = fileSystem->readFile(previousInputFilePath);
                        if (!previousSettingString.empty())
                        {
                            previousConfigurationData = ConfigurationData::fromJsonSettings(previousSettingString);
                            if (!previousConfigurationData.verifySettingsAreValid())
                            {
                                LOGDEBUG("No previous configuration data provided");
                            }
                        }
                    }
                    readSuccessful = true;
                }
                catch (Common::FileSystem::IFileSystemException& exception)
                {
                    if (readAttempt == maxReadAttempt)
                    {
                        throw Common::FileSystem::IFileSystemException(exception.what());
                    }
                }
                catch (SulDownloaderException & exception)
                {
                    if (readAttempt == maxReadAttempt)
                    {
                        throw SulDownloaderException(exception.what());
                    }
                }
                std::this_thread::sleep_for (std::chrono::seconds(1));
            } while (!readSuccessful);

            if (!configurationData.verifySettingsAreValid())
            {
                throw SulDownloaderException("Configuration data is invalid");
            }

            // If there is no previous download report, or if the download report fails to be read correctly
            // assume default download and install behaviour. i.e. install if file set has changed.
            // If overriding is required then add code to set configurationData.setForceReinstallAllProducts(true)
            DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

            try
            {
                if (!previousReportData.empty())
                {
                    previousDownloadReport = DownloadReport::toReport(previousReportData);
                }
            }
            catch (const SulDownloaderException& ex)
            {
                LOGWARN("Failed to load previous report data");
                LOGSUPPORT(ex.what());
            }

            auto report =
                runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport, supplementOnly);
            auto reportAndExitCode = DownloadReport::CodeAndSerialize(report);
            return std::tuple<int, std::string, bool>(
                std::get<0>(reportAndExitCode), std::get<1>(reportAndExitCode), report.wasBaseDowngraded());
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR(exception.what());
            if (!readSuccessful)
            {
                throw Common::FileSystem::IFileSystemException(exception.what());
            }
        }
        catch (std::exception& ex)
        {
            LOGERROR(ex.what());

        }
        auto report = DownloadReport::Report("SulDownloader failed.");
        auto reportAndExitCode = DownloadReport::CodeAndSerialize(report);
        return std::tuple<int, std::string, bool>(
            std::get<0>(reportAndExitCode), std::get<1>(reportAndExitCode), report.wasBaseDowngraded());
    }

    std::string getPreviousDownloadReportData(const std::string& outputParentPath)
    {
        // Filter file list to make sure all the files are report files based on file name
        auto previousReportFiles = DownloadReport::listOfAllPreviousReports(outputParentPath);

        std::string previousDownloadReport;

        if (!previousReportFiles.empty())
        {
            std::sort(previousReportFiles.begin(), previousReportFiles.end());

            std::string previousReportFileName = previousReportFiles.back();
            try
            {
                previousDownloadReport = Common::FileSystem::fileSystem()->readFile(previousReportFileName);
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to load previous download report file: " << previousReportFileName);
                LOGSUPPORT(ex.what());
            }
        }

        return previousDownloadReport;
    }

    int fileEntriesAndRunDownloader(
        const std::string& inputFilePath,
        const std::string& outputFilePath,
        const std::string& supplementOnlyMarkerFilePath)
    {
        auto* fileSystem = Common::FileSystem::fileSystem();

        // previous setting data will be based on location of inputFilePath.
        Path previousSettingFilePath = Common::FileSystem::join(
            Common::FileSystem::dirName(inputFilePath),
            Common::ApplicationConfiguration::applicationPathManager().getPreviousUpdateConfigFileName());


        bool supplementOnly = false;
        if (!supplementOnlyMarkerFilePath.empty() && fileSystem->isFile(supplementOnlyMarkerFilePath))
        {
            supplementOnly = true;
        }



        // check can create the output
        if (fileSystem->isDirectory(outputFilePath))
        {
            LOGERROR("Output path given is directory: " << outputFilePath);
            throw SulDownloaderException("Output file path cannot be a directory");
        }

        std::string outputParentPath = Common::FileSystem::dirName(outputFilePath);
        if (!fileSystem->isDirectory(outputParentPath))
        {
            LOGERROR("The directory of the output path does not exists: " << outputParentPath);
            throw SulDownloaderException("The directory to write the output file must exist.");
        }

        std::string previousReportData = getPreviousDownloadReportData(outputParentPath);
        int exitCode = -1;
        std::string jsonReport;
        bool baseDowngraded = false;
        std::tie(exitCode, jsonReport, baseDowngraded) =
            configAndRunDownloader(inputFilePath, previousSettingFilePath, previousReportData, supplementOnly);

        if (exitCode == 0)
        {
            LOGINFO("Update success");
        }
        else
        {
            LOGWARN("Update failed, with code: " << exitCode);
        }

        std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        LOGINFO("Generating the report file in: " << outputParentPath);

        try
        {
            Common::FileSystem::createAtomicFileWithPermissions(
                jsonReport, outputFilePath, tempDir, sophos::updateSchedulerUser(), "root", S_IRUSR | S_IWUSR);
        }
        catch (std::exception& exception)
        {
            // if failed because sophos-spl-updatescheduler doesn't exist && this is a base downgrade then write the
            // file to the old location
            if (baseDowngraded)
            {
                Common::FileSystem::createAtomicFileWithPermissions(
                    jsonReport, outputFilePath, tempDir, sophos::user(), "root", S_IRUSR | S_IWUSR);
            }
        }

        return exitCode;
    }

    int main_entry(int argc, char* argv[])
    {
        umask(S_IRWXG | S_IRWXO);
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("suldownloader", false);
        std::unique_ptr<Common::FileSystem::ILockFileHolder> pidLock;
        try
        {
            pidLock = Common::FileSystem::acquireLockFile(
                Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath());
        }
        catch (const std::system_error& ex)
        {
            LOGERROR(ex.what());
            return ex.code().value();
        }

        // Process command line args
        if (argc < 3)
        {
            LOGERROR("Error, invalid command line arguments. Usage: SULDownloader inputpath outputpath");
            return -1;
        }

        std::string inputPath = argv[1];
        std::string outputPath = argv[2];

        std::string supplementOnlyMarkerPath = Common::FileSystem::dirName(inputPath);
        supplementOnlyMarkerPath = Common::FileSystem::join(supplementOnlyMarkerPath, "supplement_only.marker");

        try
        {
            return fileEntriesAndRunDownloader(inputPath, outputPath, supplementOnlyMarkerPath);
        } // failed or unable to either read or to write files
        catch (std::exception& ex)
        {
            LOGERROR("Main exception: " << ex.what());
            return -2;
        }
        catch (...)
        {
            LOGERROR("Unknown error");
            return -3;
        }
    }

} // namespace SulDownloader
