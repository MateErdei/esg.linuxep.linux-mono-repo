/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "common/config.h"
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Susi.h>

namespace fs = sophos_filesystem;

const char* PluginName = PLUGIN_NAME;

void bootstrapperLogCallback(void*, SusiLogLevel level, const char* message)
{

    std::string m(message);
    if (!m.empty())
    {
        switch (level)
        {
            case SUSI_LOG_LEVEL_DETAIL:
                std::cout << "DEBUG: " << m;
                break;
            case SUSI_LOG_LEVEL_INFO:
                std::cout << "INFO: " << m;
                break;
            case SUSI_LOG_LEVEL_WARNING:
                std::cout << "WARNING: " << m;
                break;
            case SUSI_LOG_LEVEL_ERROR:
                std::cerr << "ERROR: " << m;
                break;
            default:
                std::cerr << level << ": " << m;
                break;
        }
    }
}

static const SusiLogCallback log_callback{
        .version = SUSI_LOG_CALLBACK_VERSION,
        .token = nullptr,
        .log = bootstrapperLogCallback,
        .minLogLevel = SUSI_LOG_LEVEL_INFO
};

int main()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PluginName;
    fs::path updateSource = pluginInstall / "chroot/susi/update_source";
    fs::path installDest = pluginInstall / "chroot/susi/distribution_version";

    auto res = SUSI_SetLogCallback(&log_callback);
    if (res != SUSI_S_OK)
    {
        std::cerr << "Failed to setup SUSI logging: " << res << std::endl;
    }

    std::cout << "Bootstrapping SUSI from update source: " << updateSource << std::endl;
    SusiResult susiResult = SUSI_Install(updateSource.c_str(), installDest.c_str());
    if (susiResult == SUSI_S_OK)
    {
        std::cout << "Successfully installed SUSI to: "  << installDest << std::endl;
    }
    else
    {
        std::cerr << "Failed to bootstrap SUSI with error: " << susiResult << std::endl;
    }
    return susiResult;
}