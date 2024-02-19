// Copyright 2024 Sophos Limited. All rights reserved.

#include "common/config.h"
#include "datatypes/sophos_filesystem.h"
#include "av/modules/safestore_extract/Extractor.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include "Common/Logging/PluginLoggingSetup.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Main/Main.h"

#include <sys/stat.h>
#include <iostream>
#include <getopt.h>
namespace fs = sophos_filesystem;

static int inner_main(int argc, char* argv[])
{
    umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "safestore_extract");
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PLUGIN_NAME;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    const auto short_opts = "sfpd";
    std::string threatPath;
    std::string password;
    std::string sha256;
    std::string unpackPath;
    const option long_opts[] = {
            { "sha256", required_argument, nullptr, 's' },
            { "threatpath", required_argument, nullptr, 'f' },
            { "password", required_argument, nullptr, 'p' },
            { "destination", required_argument, nullptr, 'd' } };
    int opt = 0;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'p':
                password = optarg;
                break;
            case 'f':
                threatPath = optarg;
                break;
            case 'd':
                unpackPath = optarg;
                break;
            case 's':
                sha256 = optarg;
                break;
            default:
                return 1;
        }
    }

    try
    {
        std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper =
                std::make_unique<safestore::SafeStoreWrapper::SafeStoreWrapperImpl>();
        safestore::Extractor e = safestore::Extractor(std::move(safeStoreWrapper));
        return e.extract(threatPath, sha256, password, unpackPath);
    }
    catch (const std::runtime_error& ex)
    {
        //error should have already been logged
        return 3;
    }
}
MAIN(inner_main(argc, argv))
