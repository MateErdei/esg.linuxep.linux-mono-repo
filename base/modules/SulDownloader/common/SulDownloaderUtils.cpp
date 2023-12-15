// Copyright 2023 Sophos Limited. All rights reserved.

#include "SulDownloaderUtils.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <sys/stat.h>

#include <map>

using namespace Common::Policy;

namespace SulDownloader
{
    bool SulDownloaderUtils::isEndpointPaused(const UpdateSettings& updateSettings)
    {
        std::vector<Common::Policy::ProductSubscription> subs = updateSettings.getProductsSubscription();
        for (const auto& subscription : subs)
        {
            if (!subscription.fixedVersion().empty())
            {
                return true;
            }
        }

        if (!updateSettings.getPrimarySubscription().fixedVersion().empty())
        {
            return true;
        }
        return false;
    }

    bool SulDownloaderUtils::checkIfWeShouldForceUpdates(const UpdateSettings& updateSettings)
    {
        auto& pathManager = Common::ApplicationConfiguration::applicationPathManager();
        auto fs = Common::FileSystem::fileSystem();

        bool paused = isEndpointPaused(updateSettings);

        std::string markerFile = pathManager.getForcedAnUpdateMarkerPath();
        if (updateSettings.getDoForcedUpdate())
        {
            if (!fs->isFile(markerFile) && !paused)
            {
                return true;
            }
        }
        else
        {
            fs->removeFile(markerFile, true);
        }

        std::string pausedMarkerFile = pathManager.getForcedAPausedUpdateMarkerPath();
        if (updateSettings.getDoPausedForcedUpdate())
        {
            if (paused && !fs->isFile(pausedMarkerFile))
            {
                return true;
            }
        }
        else
        {
            fs->removeFile(pausedMarkerFile, true);
        }

        return false;
    }

    bool SulDownloaderUtils::isProductRunning()
    {
        auto process = ::Common::Process::createProcess();
        int exitCode = 0;
        try
        {
            std::string path = Common::FileSystem::fileSystem()->getSystemCommandExecutablePath("systemctl");
            std::vector<std::string> args = { "status", "sophos-spl" };
            process->exec(path, args);
            auto status = process->wait(Common::Process::Milliseconds(100), 20);
            if (status != Common::Process::ProcessStatus::FINISHED)
            {
                LOGERROR("Timeout getting product status");
                process->kill();
            }
            auto output = process->output();
            LOGDEBUG("systemctl status sophos-spl output: " << output);
            exitCode = process->exitCode();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGERROR("Failed to check if product was running with error: " << ex.what());
            exitCode = -1;
        }
        if (exitCode == 0)
        {
            return true;
        }
        return false;
    }
    void SulDownloaderUtils::stopProduct()
    {
        auto process = ::Common::Process::createProcess();
        int exitCode = 0;
        try
        {
            std::string path = Common::FileSystem::fileSystem()->getSystemCommandExecutablePath("systemctl");
            std::vector<std::string> args = { "stop", "sophos-spl" };
            process->exec(path, args);
            auto status = process->wait(Common::Process::Milliseconds(1000), 60);
            if (status != Common::Process::ProcessStatus::FINISHED)
            {
                LOGERROR("Timeout waiting for product to stop");
                process->kill();
            }
            auto output = process->output();
            LOGDEBUG("systemctl stop output: " << output);
            exitCode = process->exitCode();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGERROR("Failed to stop product with error: " << ex.what());
            exitCode = -1;
        }
        if (exitCode != 0)
        {
            // cppcheck-suppress shiftNegative
            LOGDEBUG("systemctl status sophos-spl exit code: " << exitCode);
        }
        else
        {
            LOGINFO("Successfully stopped product");
        }
    }

    void SulDownloaderUtils::startProduct()
    {
        auto process = ::Common::Process::createProcess();
        int exitCode = 0;
        try
        {
            std::string path = Common::FileSystem::fileSystem()->getSystemCommandExecutablePath("systemctl");
            std::vector<std::string> args = { "start", "sophos-spl" };
            process->exec(path, args);
            auto status = process->wait(Common::Process::Milliseconds(1000), 60);
            if (status != Common::Process::ProcessStatus::FINISHED)
            {
                LOGERROR("Timeout waiting for product to start");
                process->kill();
            }
            auto output = process->output();
            LOGDEBUG("systemctl start output: " << output);
            exitCode = process->exitCode();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGERROR("Failed to start product with error: " << ex.what());
            exitCode = -1;
        }
        if (exitCode != 0)
        {
            LOGERROR("Failed to start product");
            // cppcheck-suppress shiftNegative
            LOGDEBUG("systemctl restart sophos-spl exit code: " << exitCode);
        }
        else
        {
            LOGINFO("Successfully started product");
        }
    }

    std::vector<std::string> SulDownloaderUtils::checkUpdatedComponentsAreRunning()
    {
        std::vector<std::string> listOfFailedProducts;
        std::map<std::string, std::string> products;
        std::string trackerFile =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderInstalledTrackerFile();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(trackerFile))
        {
            LOGINFO("Checking that updated components are running");
            std::vector<std::string> lines = fs->readLines(trackerFile);
            for (const auto& line : lines)
            {
                std::vector<std::string> parts = Common::UtilityImpl::StringUtils::splitString(line, ",");
                if (parts.size() != 2)
                {
                    LOGWARN("line " << line << " in tracker file " << trackerFile << " is malformed, discarding it");
                    continue;
                }

                if (!waitForComponentToRun(parts[0]))
                {
                    listOfFailedProducts.emplace_back(parts[1]);
                }
            }
        }
        return listOfFailedProducts;
    }

    bool SulDownloaderUtils::waitForComponentToRun(const std::string& component)
    {
        LOGINFO("Checking if component " << component << " is running");
        int i = 0;
        while (i < 30)
        {
            i++;
            if (isComponentIsRunning(component))
            {
                LOGDEBUG("Component " << component << " is running");
                return true;
            }
        }
        LOGWARN("Component " << component << " is not running");
        return false;
    }

    bool SulDownloaderUtils::isComponentIsRunning(const std::string& component)
    {
        auto process = ::Common::Process::createProcess();
        int exitCode = 0;
        try
        {
            std::string path = Common::ApplicationConfiguration::applicationPathManager().getWdctlPath();
            std::vector<std::string> args = { "isrunning", component };
            process->exec(path, args);
            auto status = process->wait(Common::Process::Milliseconds(1000), 2);
            if (status != Common::Process::ProcessStatus::FINISHED)
            {
                LOGERROR("Timeout waiting for wdctl IsRunning to return");
                process->kill();
            }
            exitCode = process->exitCode();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGERROR("Failed to check if component was running with error: " <<ex.what());
            exitCode = -1;
        }
        if (exitCode == 0)
        {
            return true;
        }

        return false;
    }

    void SulDownloaderUtils::allowUpdateSchedulerAccess(const std::string& filePath)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        bool groupExists = false;
        try
        {
            filePermissions->getGroupId(sophos::group());
            groupExists = true;
        }
        catch (const Common::FileSystem::IFileSystemException& exception)
        {
            LOGDEBUG("The group " << sophos::group() << " does not exist, indicating running via Thin Installer");
        }

        if (groupExists)
        {
            try
            {
                // Make sure Update Scheduler can access the lock file so that it can work out if suldownloader is
                // running.
                filePermissions->chown(filePath, "root", sophos::group());
                filePermissions->chmod(filePath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            }
            catch (const Common::FileSystem::IFileSystemException& exception)
            {
                LOGWARN("Could not change lock file permissions for Update Scheduler to access: " << exception.what());
            }
        }
    }
} // namespace SulDownloader