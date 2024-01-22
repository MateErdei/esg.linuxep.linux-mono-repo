// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "SulDownloader.h"

#include "ProductUninstaller.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/DownloadReport/DownloadReport.h"
#include "Common/DownloadReport/DownloadReportParseException.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IPidLockFileUtils.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Policy/PolicyParseException.h"
#include "Common/Policy/UpdateSettings.h"
#include "Common/UpdateUtilities/ConfigurationDataUtil.h"
#include "Common/UpdateUtilities/DownloadReports.h"
#include "Common/UpdateUtilities/InstalledFeatures.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "SulDownloader/sdds3/SDDS3Utils.h"
#include "SulDownloader/sdds3/Sdds3RepositoryFactory.h"
#include "SulDownloader/suldownloaderdata/ConnectionSelector.h"
#include "SulDownloader/suldownloaderdata/DownloadedProduct.h"
#include "SulDownloader/suldownloaderdata/SulDownloaderException.h"
#include "SulDownloader/suldownloaderdata/TimeTracker.h"
#include "common/Logger.h"
#include "common/SulDownloaderUtils.h"
#include "suldownloaderdata/DownloadReportBuilder.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

#include <nlohmann/json.hpp>
#include <sys/stat.h>

#include <algorithm>
#include <cassert>
#include <thread>

using namespace Common::Policy;
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
    using namespace Common::DownloadReport;
    using SulDownloader::suldownloaderdata::TimeTracker;

    void writeInstalledFeatures(const std::vector<std::string>& features)
    {
        try
        {
            Common::UpdateUtilities::writeInstalledFeaturesJsonFile(features);
        }
        catch (const nlohmann::detail::exception& jsonException)
        {
            LOGERROR(
                "The installed features list could not be serialised for persisting to disk: " << jsonException.what());
        }
        catch (const Common::FileSystem::IFileSystemException& fileSystemException)
        {
            LOGERROR("There was a problem writing the installed features list: " << fileSystemException.what());
        }
    }

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
        auto productReportItr = std::find_if(
            productReports.begin(),
            productReports.end(),
            [&product](const ProductReport& report) { return report.rigidName == product.getLine(); });

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

    DownloadReport processRepositoryAndGenerateReport(
        const bool success,
        IRepositoryPtr repository,
        TimeTracker& timeTracker,
        const UpdateSettings& configurationData,
        const DownloadReport& previousDownloadReport,
        bool forceReinstallAllProducts,
        const bool supplementOnly)
    {
        if (!success)
        {
            DownloadReport report;
            if (repository == nullptr)
            {
                report.setStatus(RepositoryStatus::DOWNLOADFAILED);
            }
            else
            {
                report = DownloadReportBuilder::Report(*repository, timeTracker);
                if (report.getProducts().empty() && report.getRepositoryComponents().empty())
                {
                    // Populate report products warehouse components from previous report, so that any issues from
                    // previously update are carried over into the next update.
                    report.combinePreviousReportIfRequired(previousDownloadReport);
                }
            }
            return report;
        }

        assert(repository != nullptr);
        assert(success);

        // These products are marked as changed only if they changed since last update or if their supplement changed
        // There is risk that we missed a product changing, and won't update/install it.
        // Also, all the returned products should be installed (not necessarily upgraded), even if not they're marked as
        // changed, as getProducts excludes products that don't match any features
        auto products = repository->getProducts();
        std::string sourceURL = repository->getSourceURL();

        auto fileSystem = Common::FileSystem::fileSystem();

        // TODO: to be removed once no customer is stuck on scheduled updates due to LINUXDAR-6907
        // If this flag exists, that means we need to avoid doing supplement updates until the next scheduled update
        // This is to avoid out-of-schedule product updates before the first scheduled update, e.g. due to supplement
        // updates.
        const auto awaitSupplementUpdateFlagPath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
            "base/update/var/updatescheduler/await_scheduled_update");
        // Only do the above behaviour if installed base version is older than 1.2.3, otherwise the issue isn't there
        const auto baseVersionIniPath = Common::ApplicationConfiguration::applicationPathManager().getVersionFilePath();
        bool isVersionOlderThan123 = true;
        try
        {
            const auto baseVersion = StringUtils::extractValueFromIniFile(baseVersionIniPath, "PRODUCT_VERSION");
            isVersionOlderThan123 = StringUtils::isVersionOlder("1.2.3", baseVersion);
        }
        catch (const std::runtime_error& e)
        {
            LOGDEBUG("Can't determine if base version is newer than 1.2.3: " << e.what());
        }

        // for writing marker files if we do a forced update trigger by flag
        bool writeForceMarkerFile = false;
        std::string forcedUpdateMarkerFilePath;
        if (supplementOnly)
        {
            if (fileSystem->isFile(awaitSupplementUpdateFlagPath) && isVersionOlderThan123)
            {
                LOGINFO("Waiting for next scheduled update.");
                for (auto& product : products)
                {
                    product.setProductHasChanged(false);
                }
            }
        }
        else
        {
            fileSystem->removeFile(awaitSupplementUpdateFlagPath, true);
            if (SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData))
            {
                LOGINFO("Triggering a force reinstall");
                forceReinstallAllProducts = true;
                writeForceMarkerFile = true;
                if (SulDownloaderUtils::isEndpointPaused(configurationData))
                {
                    forcedUpdateMarkerFilePath =
                        Common::ApplicationConfiguration::applicationPathManager().getForcedAPausedUpdateMarkerPath();
                }
                else
                {
                    forcedUpdateMarkerFilePath =
                        Common::ApplicationConfiguration::applicationPathManager().getForcedAnUpdateMarkerPath();
                }
            }
        }

        for (auto& product : products)
        {
            std::string rigidName = product.getProductMetadata().getLine();
            LOGDEBUG("Checking if " << rigidName << " needs to be downgraded");

            bool isCachedVersionNewerThanInstalled = false;

            try
            {
                std::string localVersionIni =
                    Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(rigidName);
                if (!Common::FileSystem::fileSystem()->isFile(localVersionIni))
                {
                    // if local version.ini doesn't exist assume plugin is an older version than XDR EAP or not
                    // installed therefore no downgrade
                    LOGDEBUG("Plugin " << rigidName << " in warehouse is newer than version on disk");
                    isCachedVersionNewerThanInstalled = true;
                }
                else
                {
                    std::string currentVersion =
                        StringUtils::extractValueFromIniFile(localVersionIni, "PRODUCT_VERSION");
                    std::string newVersion = product.getProductMetadata().getVersion();

                    if (newVersion.empty() || currentVersion.empty())
                    {
                        // if newVersion is empty could be due the product not being able to read the file
                        // if currentVersion is empty something went wrong with the install possibly
                        // therefore we would like to do a downgrade to get as close to a fresh install as possible
                        product.setProductWillBeDowngraded(true);
                    }
                    else
                    {
                        LOGDEBUG(
                            "Component " << rigidName << " current version: " << currentVersion
                                         << ", new version: " << newVersion);
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

                        if (StringUtils::isVersionOlder(newVersion, currentVersion))
                        {
                            isCachedVersionNewerThanInstalled = true;
                        }
                    }
                }
            }
            catch (std::runtime_error& ex)
            {
                // run time error thrown if local version.ini file missing
                LOGWARN("Failed to open version ini file with error: " << ex.what());
                isCachedVersionNewerThanInstalled = true;
            }
            catch (std::invalid_argument& ex)
            {
                // invalid argument thrown by isVersionOlder if Version string are not in the right format
                LOGWARN("Failed to calculate version difference due to: " << ex.what());
                isCachedVersionNewerThanInstalled = true;
            }

            if (!supplementOnly && isCachedVersionNewerThanInstalled)
            {
                product.setProductHasChanged(true);
            }
        }
        bool productChanging = false;
        for (auto& product : products)
        {
            bool forceReinstallThisProduct = forceInstallOfProduct(product, previousDownloadReport);

            if (forceReinstallAllProducts || forceReinstallThisProduct)
            {
                LOGDEBUG(
                    "Mark product to be reinstalled. Reason: AllProducts: "
                    << forceReinstallAllProducts << ", This Product: " << forceReinstallThisProduct
                    << ". Product = " << product.getLine());
                product.setForceProductReinstall(true);
                productChanging = true;
            }
        }
        LOGDEBUG("Checking signature.");
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
            return DownloadReportBuilder::Report(
                sourceURL,
                products,
                {},
                repository->listInstalledSubscriptions(),
                &timeTracker,
                DownloadReportBuilder::VerifyState::VerifyFailed,
                false);
        }

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
        std::string trackerFile =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderInstalledTrackerFile();
        fileSystem->removeFile(trackerFile, true);
        bool fullRestart = false;
        bool copyConfig = false;

        // Use environment variable to determine if SPL should be restarted after upgrade
        setenv("SOPHOS_INHIBIT_WATCHDOG_RESTART", "true", 1);
        for (auto& product : products)
        {
            if ((product.productHasChanged() || product.forceProductReinstall()) &&
                !product.getProductIsBeingUninstalled())
            {
                if ((product.getLine() == "ServerProtectionLinux-Base-component"))
                {
                    if (SulDownloaderUtils::isProductRunning())
                    {
                        LOGDEBUG("Triggering stopProduct");
                        SulDownloaderUtils::stopProduct();
                    }
                    else
                    {
                        LOGDEBUG("This is a fresh install so not triggering product stop before installing component");
                        copyConfig = true;
                    }
                    fullRestart = true;
                }
                product.install(configurationData.getInstallArguments());
                if (copyConfig)
                {
                    try
                    {
                        // copy config generated by thin installer into place for update scheduler to read it
                        if (fileSystem->isFile("update_config.json"))
                        {
                            std::string content = fileSystem->readFile("update_config.json");
                            Common::FileSystem::createAtomicFileWithPermissions(
                                content,
                                Common::ApplicationConfiguration::applicationPathManager()
                                    .getSulDownloaderConfigFilePath(),
                                Common::ApplicationConfiguration::applicationPathManager().getTempPath(),
                                sophos::updateSchedulerUser(),
                                sophos::group(),
                                S_IRUSR | S_IWUSR | S_IRGRP);
                        }
                    }
                    catch (const Common::FileSystem::IFileSystemException& exception)
                    {
                        LOGWARN(
                            "Failed to copy update config generated by thininstaller into standard location with "
                            "error: "
                            << exception.what());
                    }
                    copyConfig = false;
                }

                // If base failed to install, don't attempt to install anything else
                if (product.getLine() == "ServerProtectionLinux-Base-component" && product.hasError())
                {
                    LOGERROR("Base installation failed, not installing any plugins");
                    break;
                }
            }
            else
            {
                LOGINFO("Downloaded Product line: '" << product.getLine() << "' is up to date.");
            }
        }

        if (fullRestart && !SulDownloaderUtils::isProductRunning())
        {
            LOGDEBUG("Triggering startProduct");
            SulDownloaderUtils::startProduct();
        }

        std::vector<std::string> failedProducts = SulDownloaderUtils::checkUpdatedComponentsAreRunning();
        for (const auto& failed : failedProducts)
        {
            for (auto& product : products)
            {
                if (product.getLine() == failed)
                {
                    RepositoryError error;
                    error.Description = std::string("Product ") + product.getLine() + " failed to install";
                    error.status = RepositoryStatus::INSTALLFAILED;
                    product.setError(error);
                    continue;
                }
            }
        }
        fileSystem->removeFile(trackerFile, true);

        timeTracker.setFinishedTime(TimeUtils::getCurrTime());
        if (productChanging)
        {
            try
            {
                std::string updateMarkerFile =
                    Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
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
        if (writeForceMarkerFile)
        {
            try
            {
                fileSystem->writeFile(forcedUpdateMarkerFilePath, "");
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to write reinstall marker file with error: " << ex.what());
            }
        }
        // if any error happened during installation, it reports correctly.
        // the report also contains the successful ones.

        LOGDEBUG("Triggering purge");
        repository->purge();
        return DownloadReportBuilder::Report(
            sourceURL,
            products,
            repository->listInstalledProducts(),
            repository->listInstalledSubscriptions(),
            &timeTracker,
            DownloadReportBuilder::VerifyState::VerifyCorrect,
            supplementOnly,
            setForceInstallForAllProducts);
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

    std::pair<bool, IRepositoryPtr> updateFromSDDS3Repository(
        const UpdateSettings& updateSettings,
        const bool supplementOnly,
        const DownloadReport& previousDownloadReport,
        const bool forceReinstallAllProducts)
    {
        ConnectionSelector connectionSelector;
        auto [susCandidates, connectionCandidates] = connectionSelector.getConnectionCandidates(updateSettings);
        auto repository = Sdds3RepositoryFactory::instance().createRepository();
        // sus
        for (auto& connectionCandidate : susCandidates)
        {
            LOGDEBUG(
                "Trying connection candidate, URL: " << connectionCandidate.getUpdateLocationURL()
                                                     << ", proxy: " << connectionCandidate.getProxy().getUrl());
            if (repository->tryConnect(connectionCandidate, supplementOnly, updateSettings))
            {
                break;
            }
        }

        if (repository->hasError())
        {
            LOGERROR("Failed to connect to repository: " << repository->getError().Description);
            return std::make_pair(false, std::move(repository));
        }

        LOGDEBUG("Purging local SDDS2 cache");
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            if (fileSystem->isDirectory(updateSettings.getLocalWarehouseRepository()))
            {
                fileSystem->recursivelyDeleteContentsOfDirectory(updateSettings.getLocalWarehouseRepository());
            }
            if (fileSystem->isDirectory(updateSettings.getLocalDistributionRepository()))
            {
                fileSystem->recursivelyDeleteContentsOfDirectory(updateSettings.getLocalDistributionRepository());
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to delete SDDS2 cache, reason:" << ex.what());
        }

        // using iterator here instead of a for range to avoid ignoringSupplementFailure early
        //  if there is a duplicate in the list for the last candidate
        for (auto it = connectionCandidates.begin(); it != connectionCandidates.end(); ++it)
        {
            if (repository->synchronize(updateSettings, *it, false))
            {
                break;
            }
        }

        if (repository->hasError())
        {
            LOGERROR("Failed to synchronize repository: " << repository->getError().Description);
            return std::make_pair(false, std::move(repository));
        }

        // this section is to check if we want to reinstall any of the packages when they haven't changed,
        // so we need to set setDoUnpackRepository so the repository is unpacked
        if (forceReinstallAllProducts)
        {
            repository->setDoUnpackRepository(true);
        }
        else
        {
            auto products = repository->getProducts();
            for (auto& product : products)
            {
                bool forceReinstallThisProduct = forceInstallOfProduct(product, previousDownloadReport);

                if (forceReinstallThisProduct)
                {
                    repository->setDoUnpackRepository(true);
                }
            }
        }

        repository->distribute();
        if (repository->hasError())
        {
            LOGERROR("Failed to distribute repository: " << repository->getError().Description);
            return std::make_pair(false, std::move(repository));
        }
        return std::make_pair(true, std::move(repository));
    }

    DownloadReport runSULDownloader(
        const UpdateSettings& updateSettings,
        const UpdateSettings& previousUpdateSettings,
        const DownloadReport& previousDownloadReport,
        bool supplementOnly)
    {
        // Mark which products need to be forced to re/install.
        bool forceReinstallAllProducts =
            Common::UpdateUtilities::ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(
                updateSettings, previousUpdateSettings, false);
        assert(updateSettings.isVerified());

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

        std::pair<bool, IRepositoryPtr> repositoryResult = std::make_pair(false, nullptr);

        if (!updateSettings.getPrimarySubscription().fixedVersion().empty() &&
            StringUtils::isVersionOlder("2022", updateSettings.getPrimarySubscription().fixedVersion()))
        {
            LOGERROR(
                "The requested fixed version is not available on SDDS3: " +
                updateSettings.getPrimarySubscription().fixedVersion() + ". Package too old.");
        }
        else
        {
            if (!updateSettings.getJWToken().empty())
            {
                LOGINFO("Running SDDS3 update");
                // Make sure root directories are created
                createSdds3UpdateCacheFolders();
                repositoryResult = updateFromSDDS3Repository(
                    updateSettings, supplementOnly, previousDownloadReport, forceReinstallAllProducts);
            }
            else
            {
                LOGERROR("Failed to update because JWToken was empty");
            }
        }

        return processRepositoryAndGenerateReport(
            repositoryResult.first,
            std::move(repositoryResult.second),
            timeTracker,
            updateSettings,
            previousDownloadReport,
            forceReinstallAllProducts,
            supplementOnly);
    }

    std::tuple<int, std::string, bool> configAndRunDownloader(
        const std::string& inputFilePath,
        const std::string& previousInputFilePath,
        const std::string& previousReportData,
        bool supplementOnly,
        std::chrono::milliseconds readFailedRetryInterval,
        int maxReadAttempts)
    {
        bool readSuccessful = false;
        auto report = DownloadReportBuilder::Report("SulDownloader failed.");
        auto* fileSystem = Common::FileSystem::fileSystem();
        try
        {
            int readAttempt = 0;

            UpdateSettings updateSettings;
            UpdateSettings previousUpdateSettings;
            do
            {
                readAttempt++;
                try
                {
                    std::string settingsString = fileSystem->readFile(inputFilePath);
                    LOGDEBUG("Configure and run downloader: " << settingsString);

                    updateSettings = ConfigurationData::fromJsonSettings(settingsString);

                    if (fileSystem->isFile(previousInputFilePath))
                    {
                        std::string previousSettingString = fileSystem->readFile(previousInputFilePath);
                        if (!previousSettingString.empty())
                        {
                            previousUpdateSettings = ConfigurationData::fromJsonSettings(previousSettingString);
                            if (!previousUpdateSettings.verifySettingsAreValid())
                            {
                                LOGDEBUG("No previous configuration data provided");
                            }
                        }
                    }
                    readSuccessful = true;
                    break;
                }
                catch (const Common::FileSystem::IFileSystemException& exception)
                {
                    if (readAttempt == maxReadAttempts)
                    {
                        std::throw_with_nested(Common::FileSystem::IFileSystemException(LOCATION, exception.what()));
                    }
                }
                catch (const Common::Policy::PolicyParseException& exception)
                {
                    if (readAttempt == maxReadAttempts)
                    {
                        std::throw_with_nested(Common::Policy::PolicyParseException(LOCATION, exception.what()));
                    }
                }
                if (!readSuccessful)
                {
                    std::this_thread::sleep_for(readFailedRetryInterval);
                }
            } while (!readSuccessful);

            if (!updateSettings.verifySettingsAreValid())
            {
                throw SulDownloaderException(LOCATION, "Configuration data is invalid");
            }

            // If there is no previous download report, or if the download report fails to be read correctly
            // assume default download and install behaviour. i.e. install if file set has changed.
            // If overriding is required then add code to set configurationData.setForceReinstallAllProducts(true)
            DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

            try
            {
                if (!previousReportData.empty())
                {
                    previousDownloadReport = DownloadReport::toReport(previousReportData);
                }
            }
            catch (const Common::DownloadReport::DownloadReportParseException& ex)
            {
                LOGWARN("Failed to load previous report data");
                LOGSUPPORT(ex.what());
            }

            report = runSULDownloader(updateSettings, previousUpdateSettings, previousDownloadReport, supplementOnly);

            // If the installation was successful then we can safely update the list of installed features
            // We don't want to write the features on a supplement only update, since it isn't guaranteed that all the
            // requested features were installed (e.g. the cached SUS response doesn't have all the suites)
            if (report.getExitCode() == 0 && !supplementOnly)
            {
                writeInstalledFeatures(updateSettings.getFeatures());
            }
        }
        catch (const Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to run Suldownloader with file system error: " << exception.what());
            if (!readSuccessful)
            {
                throw Common::FileSystem::IFileSystemException(exception.what());
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Failed to run Suldownloader with error: " << ex.what());
        }

        auto reportAndExitCode = DownloadReport::CodeAndSerialize(report);
        return std::tuple<int, std::string, bool>(
            std::get<0>(reportAndExitCode), std::get<1>(reportAndExitCode), report.wasBaseDowngraded());
    }

    std::string getPreviousDownloadReportData(const std::string& outputParentPath)
    {
        // Filter file list to make sure all the files are report files based on file name
        auto previousReportFiles = Common::UpdateUtilities::listOfAllPreviousReports(outputParentPath);

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
        const std::string& supplementOnlyMarkerFilePath,
        std::chrono::milliseconds readFailedRetryInterval)
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
        std::tie(exitCode, jsonReport, baseDowngraded) = configAndRunDownloader(
            inputFilePath, previousSettingFilePath, previousReportData, supplementOnly, readFailedRetryInterval);

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

    int main_entry(int argc, char* argv[], std::chrono::milliseconds readFailedRetryInterval)
    {
        umask(S_IRWXG | S_IRWXO);
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("suldownloader", false);
        LOGINFO("SulDownloader " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");

        std::unique_ptr<Common::FileSystem::ILockFileHolder> pidLock;
        try
        {
            pidLock = Common::FileSystem::acquireLockFile(
                Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath());
            LOGDEBUG("Lockfile: " << pidLock->filePath());
        }
        catch (const std::system_error& ex)
        {
            LOGERROR("Could not acquire lock file: " << ex.what());
            return ex.code().value();
        }

        SulDownloaderUtils::allowUpdateSchedulerAccess(pidLock->filePath());

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
            return fileEntriesAndRunDownloader(
                inputPath, outputPath, supplementOnlyMarkerPath, readFailedRetryInterval);
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
