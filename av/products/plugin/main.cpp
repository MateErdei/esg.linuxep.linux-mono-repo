// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "common/config.h"

#include "datatypes/sophos_filesystem.h"

#include "pluginimpl/Logger.h"
#include "pluginimpl/PluginAdapter.h"
#ifdef SPL_BAZEL
#    include "av/AutoVersion.h"
#else
#    include "AutoVersioningHeaders/AutoVersion.h"
#endif

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Logging/PluginLoggingSetup.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/ErrorCodes.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginApi/IPluginResourceManagement.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/ZeroMQWrapper/IIPCTimeoutException.h"

static const char* PluginName = PLUGIN_NAME; // NOLINT

namespace fs = sophos_filesystem;

int main()
{
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PluginName;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(PluginName);
    LOGINFO("AV Plugin " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto taskQueue = std::make_shared<TaskQueue>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(taskQueue);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;

    try
    {
        // Also does Telemetry restore for PluginName
        // If this is the first time restoring this will also set the telemetry backup file name
        baseService = resourceManagement->createPluginAPI(PluginName, sharedPluginCallBack);
    }
    catch (const Common::PluginApi::ApiException & apiException)
    {
        LOGERROR("Failed to instantiate Plugin Api: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }
    catch (const Common::ZeroMQWrapper::IIPCTimeoutException& ex)
    {
        LOGERROR("Failed to instantiate Plugin Api (IIPCTimeoutException): " << ex.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }
    catch (const Common::ZeroMQWrapper::IIPCException& ex)
    {
        LOGERROR("Failed to instantiate Plugin Api (IIPCException): " << ex.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }
    catch (const Common::Exceptions::IException& ex)
    {
        LOGERROR("Failed to instantiate Plugin Api (IException): " << ex.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Failed to instantiate Plugin Api (std::exception): " << ex.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    fs::path threatEventPublisherSocketPath = pluginInstall / "var/threatEventPublisherSocketPath";
    PluginAdapter pluginAdapter(taskQueue, std::move(baseService), sharedPluginCallBack, threatEventPublisherSocketPath);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const Common::PluginApi::ApiException & apiException)
    {
        LOGERROR("Exception caught from plugin at top level (ApiException): " << apiException.what_with_location());
        ret = 45;
    }
    catch (const Common::ZeroMQWrapper::IIPCTimeoutException& ex)
    {
        LOGERROR("Exception caught from plugin at top level (IIPCTimeoutException): " << ex.what_with_location());
        ret = 44;
    }
    catch (const Common::ZeroMQWrapper::IIPCException& ex)
    {
        LOGERROR("Exception caught from plugin at top level (IIPCException): " << ex.what_with_location());
        ret = 43;
    }
    catch (const Common::Exceptions::IException& ex)
    {
        LOGERROR("Exception caught from plugin at top level (IException): " << ex.what_with_location());
        ret = 42;
    }
    catch (const std::runtime_error& ex)
    {
        LOGERROR("Exception caught from plugin at top level (std::runtime_error): " << ex.what());
        ret = 41;
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Exception caught from plugin at top level (std::exception): " << ex.what());
        ret = 40;
    }

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    telemetry.set("threatHealth",sharedPluginCallBack->getThreatHealth());
    telemetry.save();

    LOGINFO("Exiting AV plugin with " << ret);
    sharedPluginCallBack->setRunning(false);
    return ret;
}
