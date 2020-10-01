/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosThreatDetectorMain.h"
#include "Logger.h"

#include <fstream>
#include "common/Define.h"
#ifdef USE_SUSI
#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>
#else
#include <sophos_threat_detector/threat_scanner/FakeSusiScannerFactory.h>
#endif
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <string>
#include <unistd.h>

using namespace sspl::sophosthreatdetectorimpl;
namespace fs = sophos_filesystem;

static void copyRequiredFiles(const fs::path& sophosInstall, const fs::path& chrootPath)
{
    std::vector<std::string> fileVector;
    fileVector.emplace_back("/base/etc/logger.conf");
    fileVector.emplace_back("/base/etc/machine_id.txt");
    fileVector.emplace_back("/base/mcs/policy/ALC-1_policy.xml");

    for (const std::string& file : fileVector)
    {
        fs::path sourceFile = sophosInstall / file;
        fs::path targetFile = chrootPath / sophosInstall / file;
        LOGINFO("Copying " << sourceFile.string() << " to: " << targetFile.string());

        try
        {
            fs::copy_file(sourceFile, targetFile, fs::copy_options::overwrite_existing);
        }
        catch (fs::filesystem_error& e)
        {
            LOGERROR("Failed to copy: " << file);
        }
    }
}

static int inner_main()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path chrootPath = pluginInstall / "chroot";
    LOGDEBUG("Preparing to enter chroot at: " << chrootPath);
#ifdef USE_CHROOT
    // Copy logger config from base
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    copyRequiredFiles(sophosInstall, chrootPath);

    int ret = ::chroot(chrootPath.c_str());
    if (ret != 0)
    {
        LOGERROR("Failed to chroot to " << chrootPath.c_str() << " (" << errno << "): Check permissions");
        exit(EXIT_FAILURE);
    }

    fs::path scanningSocketPath = "/scanning_socket";
#else
    fs::path scanningSocketPath = chrootPath / "scanning_socket";
#endif

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
#ifdef USE_SUSI
        = std::make_shared<threat_scanner::SusiScannerFactory>();
#else
        = std::make_shared<threat_scanner::FakeSusiScannerFactory>();
#endif
    unixsocket::ScanningServerSocket server(scanningSocketPath, 0666, scannerFactory);
    server.run();

    return 0;
}

int sspl::sophosthreatdetectorimpl::sophos_threat_detector_main()
{
    try
    {
        return inner_main();
    }
    catch (std::exception& ex)
    {
        LOGERROR("Caught std::exception: "<<ex.what() << " at top level");
        return 101;
    }
    catch(...)
    {
        LOGERROR("Caught unknown exception at top-level");
        return 100;
    }
}

