/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IPidLockFileUtils.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/Logging/PluginLoggingSetupEx.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Proc/ProcUtilities.h>
#include <modules/livequery/config.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/Logger.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <unistd.h>

const char* g_pluginName = PLUGIN_NAME;

int main()
{
    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(g_pluginName);
    Common::Logging::PluginLoggingSetupEx scheduledQueryLoggerSetup(g_pluginName, "scheduledquery", "scheduledquery");
    Common::Logging::PluginLoggingSetupEx osqueryLoggerSetup(g_pluginName, "edr_osquery", "edr_osquery");
    std::unique_ptr<Common::FileSystem::ILockFileHolder> lockFile;
    try
    {
        lockFile = Common::FileSystem::acquireLockFile(lockFilePath());
    }
    catch (std::system_error & ex)
    {
        LOGWARN("EDR lock file already locked, checking other EDR processes");
        std::vector<std::string> entries = Common::FileSystem::fileSystem()->listFilesAndDirectories("/proc");

        auto fs = Common::FileSystem::fileSystem();
        for (const auto& entry : entries)
        {
            auto basename = Common::FileSystem::basename(entry);

            std::stringstream numbStr(basename);
            long int longInt;
            numbStr >> longInt;
            if (numbStr.fail())
            {
                continue;
            }

            std::optional<long> pid = longInt;

            if (pid.has_value() && pid.value() != getpid())
            {
                std::string pathToProcExe = Common::FileSystem::join(entry, "exe");
                std::string absolutePath = fs->readlink(pathToProcExe);

                if (Common::UtilityImpl::StringUtils::startswith(absolutePath, Plugin::edrBinaryPath()))
                {
                    LOGINFO("Found running EDR instance with PID: " << pid.value());
                    std::optional<std::string> statContent = fs->readProcFile(pid.value(), "stat");
                    if (statContent.has_value())
                    {
                        auto procInfo = Proc::parseProcStat(statContent.value());
                        if (procInfo.value().ppid == 1)
                        {
                            LOGINFO("Detected EDR running but not managed by WD, terminating process: " << pid.value());
                            Proc::killProcess(pid.value());
                        }
                    }
                }

            }
        }
        // Rely on the WD to restart EDR knowing that all orphaned EDR
        // processes that erroneously held the lock file are gone
        return ex.code().value();
    }

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto queueTask = std::make_shared<QueueTask>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;

    try
    {
        baseService = resourceManagement->createPluginAPI(g_pluginName, sharedPluginCallBack);
    }
    catch (const Common::PluginApi::ApiException & apiException)
    {
        LOGERROR("Plugin Api could not be instantiated: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
        ret = 40;
    }
    sharedPluginCallBack->setRunning(false);
    LOGINFO("Plugin Finished.");
    return ret;
}
